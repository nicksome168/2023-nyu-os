#include "cmdreader.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

// how to use getline: https://c-for-dummies.com/blog/?p=1112s
// how to use getcwd: https://iq.opengenus.org/chdir-fchdir-getcwd-in-c/
// how to continuously getchar() from a line: https://stackoverflow.com/questions/33947693/why-does-getchar-continue-to-take-characters-from-a-line-of-input-instead-of-a
// how to copy chars: https://stackoverflow.com/questions/6205195/given-a-starting-and-ending-indices-how-can-i-copy-part-of-a-string-in-c
// include wait library: https://stackoverflow.com/questions/41884685/implicit-declaration-of-function-wait
// How to use execv with a generated path in C?: https://stackoverflow.com/questions/52240612/how-to-use-execv-with-a-generated-path-in-c
// How to make parent wait for all child processes to finish?: https://stackoverflow.com/questions/19461744/how-to-make-parent-wait-for-all-child-processes-to-finish
// How to use strtok: https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
// How to ignore signal in parent but not in child process: https://stackoverflow.com/questions/74522774/how-a-parent-process-can-ignore-sigint-and-child-process-doesnt
// How to return to parent after child process stop: https://stackoverflow.com/questions/39962707/wait-does-not-return-after-child-received-sigstop
// How to redirect i/o: https://stackoverflow.com/questions/19846272/redirecting-i-o-implementation-of-a-shell-in-c
size_t CMD_BUFF_MAX = 1000;

void get_curdir(char *abs_path, char *relat_path);
int my_system(char *command);
int is_builtin_cmd(char *command);
int builtin_cmd_handler(char *command);
void invalid_cmd(char *err_msg);
void _io_redir_handler(char **argv, int arrow_pos, int io_flag);
int _parse_command_line(char *command, char **argv);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int main()
{
    int characters = 0;
    char *abs_path = (char *)malloc(CMD_BUFF_MAX * sizeof(char));
    char *relat_path = (char *)malloc(CMD_BUFF_MAX * sizeof(char));
    char *cmd_buffer = (char *)malloc(CMD_BUFF_MAX * sizeof(char));
    // ignore certain signals
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    while (1)
    {
        getcwd(abs_path, CMD_BUFF_MAX * sizeof(char));
        get_curdir(abs_path, relat_path);
        // print prompt
        printf("[nyush %s]$ ", relat_path);
        fflush(stdout);
        characters = getline(&cmd_buffer, &CMD_BUFF_MAX, stdin);
        cmd_buffer[characters - 1] = '\0'; // remove newline
        if (characters == -1)              // ctrl+d
        {
            break;
        }
        else if (characters == 1) // enter
        {
            continue;
        }
        else if (is_builtin_cmd(cmd_buffer))
        {
            if (builtin_cmd_handler(cmd_buffer) == -1)
                break;
        }
        else
        {
            my_system(cmd_buffer);
        }
    }
    free(abs_path);
    free(relat_path);
    free(cmd_buffer);
}

void get_curdir(char *abs_path, char *relat_path)
{
    // if it's root
    if (strlen(abs_path) == 1)
    {
        strcpy(relat_path, abs_path);
    }
    else
    {
        // if it's a dir under root
        for (int i = strlen(abs_path) - 1; i >= 0; i--)
        {
            if (abs_path[i] == '/')
            {
                strcpy(relat_path, abs_path + i + 1);
                break;
            }
        }
    }
}

void invalid_cmd(char *err_msg)
{
    fprintf(stderr, err_msg);
    fflush(stdout);
}

int is_builtin_cmd(char *command)
{
    if (strstr(command, "cd") || strstr(command, "jobs") || strstr(command, "fg") || strstr(command, "exit"))
        return 1;
    return 0;
}

int builtin_cmd_handler(char *command)
{
    strtok(command, " ");          // cmd itself
    char *arg = strtok(NULL, " "); // first arg
    if (strstr(command, "jobs") || strstr(command, "exit"))
    {
        // should take no argument
        if (arg != NULL)
        {
            invalid_cmd("Error: invalid command\n");
            return 0;
        }
        if (strstr(command, "exit"))
        {
            return -1;
        }
        else
        {
        }
    }
    else if (strstr(command, "cd") || strstr(command, "fg"))
    {
        // should not take 0 or 2+ arguments
        if (arg == NULL || strtok(NULL, " ") != NULL)
        {
            invalid_cmd("Error: invalid command\n");
            return 0;
        }
        if (strstr(command, "cd"))
        {
            if (chdir(arg) < 0)
            {
                invalid_cmd("Error: invalid directory\n");
            }
        }
        else
        {
        }
    }
    return 0;
}

void _io_redir_handler(char **argv, int arrow_pos, int io_flag)
{
    char *file;
    if (argv[arrow_pos + 1] != NULL)
    {
        file = malloc(strlen(argv[arrow_pos + 1]) * sizeof(char));
        strcpy(file, argv[arrow_pos + 1]);
        int fd;
        if (io_flag) // output
        {
            // printf("wrote output to %s\n", file);
            fd = open(file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
            dup2(fd, 1);
        }
        else // input
        {
            //  printf("wrote input to %s\n", file);
            fd = open(file, O_RDONLY);
            if (fd < 0) // check if file exists
                invalid_cmd("Error: invalid file\n");
            dup2(fd, 0);
        }
        close(fd);
        free(file);
    }
    else // no file specified
    {
        invalid_cmd("Error: invalid command\n");
    }
}

int _parse_command_line(char *command, char **argv)
{
    char *slash_pos = strchr(command, '/');                        // strrchr(target,key): find the first key and return the pointer or NULL
    char *prog_path = (char *)malloc(CMD_BUFF_MAX * sizeof(char)); // construct program's full path
    char *arg = strtok(command, " ");                              // split command by space
    if (slash_pos == NULL)                                         // only with base name
        strcat(prog_path, "/usr/bin/");
    strcat(prog_path, arg);
    int argc = 0;
    argv[argc] = prog_path; //{prog_path, prog_name, arg1, arg2, ..., NULL}
    argc++;
    int in_redirect = 0;
    int out_redirect = 0;
    while (arg) // add arg to argv
    {
        arg = strtok(NULL, " ");
        if (arg != NULL)
        {
            if (strstr(arg, "<"))
                in_redirect = argc;
            else if (strstr(arg, ">") || strstr(arg, ">>"))
                out_redirect = argc;
        }
        argv[argc] = arg;
        argc++;
    };
    // for (int i = 0; argv[i] != NULL; i++)
    //     printf("argv %d: %s\n", i, argv[i]);
    // printf("in_redirect: %d\n", in_redirect);
    // printf("out_redirect: %d\n", out_redirect);
    // handle input redirect
    if (in_redirect)
        _io_redir_handler(argv, in_redirect, 0);
    // handle output  redirect
    if (out_redirect)
        _io_redir_handler(argv, out_redirect, 1);
    if (in_redirect || out_redirect)
        return (MIN(in_redirect, out_redirect) != 0 ? MIN(in_redirect, out_redirect) : MAX(in_redirect, out_redirect));
    return 0;
}

int my_system(char *command)
{
    char *argv[100];
    int pid = fork();
    if (pid < 0)
    {
        // fork failed (this shouldn't happen)
        invalid_cmd("Error: fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        int has_io_redir = _parse_command_line(command, argv); // return 0 if no i/o redir, else return the first arrow position
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        if (has_io_redir)
        {
            argv[has_io_redir] = NULL;
            execv(argv[0], argv);
        }
        else
        {
            execv(argv[0], argv);
        }
        invalid_cmd("Error: invalid program\n");
        exit(1);
    }
    // parent waits for the children process
    waitpid(-1, NULL, WUNTRACED);
    return 0;
}