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
// how to include wait library: https://stackoverflow.com/questions/41884685/implicit-declaration-of-function-wait
// how to use execv with a generated path in C?: https://stackoverflow.com/questions/52240612/how-to-use-execv-with-a-generated-path-in-c
// how to make parent wait for all child processes to finish?: https://stackoverflow.com/questions/19461744/how-to-make-parent-wait-for-all-child-processes-to-finish
// how to use strtok: https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
// how to ignore signal in parent but not in child process: https://stackoverflow.com/questions/74522774/how-a-parent-process-can-ignore-sigint-and-child-process-doesnt
// how to return to parent after child process stop: https://stackoverflow.com/questions/39962707/wait-does-not-return-after-child-received-sigstop
// how to redirect i/o: https://stackoverflow.com/questions/19846272/redirecting-i-o-implementation-of-a-shell-in-c
// how to create multiple child processes: https://stackoverflow.com/questions/876605/multiple-child-process
// how to implement multiple pipes: https://stackoverflow.com/questions/8389033/implementation-of-multiple-pipes-in-c
// how to implement jobs struct: https://www.gnu.org/software/libc/manual/html_node/Data-Structures.html
// how to initialize a struct in c: https://stackoverflow.com/questions/330793/how-to-initialize-a-struct-in-accordance-with-c-programming-language-standards
// hwo to acessing a locally declared struct outside of its scope: https://stackoverflow.com/questions/27763407/acessing-a-locally-declared-struct-outside-of-its-scope

// define process struct
typedef struct process
{
    struct process *_next;
    int _pid;
    int _fildes[2];
    char *_cmd;
} process;

// define jobs struct
typedef struct jobs
{
    struct process *_head_proc; // the head of the list
    struct process *_tail_proc; // the tail of the list
} jobs;

struct jobs _ALL_JOBS = {._head_proc = NULL, ._tail_proc = NULL};
struct jobs *ALL_JOBS = &_ALL_JOBS;

size_t CMD_BUFF_MAX = 1000;
size_t ARG_MAX = 10; // assume no more than ARG_MAX arguments

void get_curdir(char *abs_path, char *relat_path);
int my_system(char *command);
int is_builtin_cmd(char *command);
int builtin_cmd_handler(char *command);
void invalid_cmd(char *err_msg);
void _io_redir_handler(char **argv, int arrow_pos, int io_flag);
void _redir_io(char **argv);
void _add_path(char **subarg);
void _split_cmd(char **cmdv, char ***cmdvv);
int _split_pipe(char *command, char **cmdv);

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
    struct process *ptr = ALL_JOBS->_head_proc;
    struct process *nxt_ptr;
    while (ptr)
    {
        free(ptr->_cmd);
        nxt_ptr = ptr->_next;
        free(ptr);
        ptr = nxt_ptr;
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
        else // jobs cmd
        {
            // TODO handle print jobs
            int count = 1;
            struct process *p_ptr = ALL_JOBS->_head_proc;
            while (p_ptr)
            {
                printf("[%d] %s\n", count, p_ptr->_cmd);
                count++;
                p_ptr = p_ptr->_next;
            }
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
        else // fg cmd
        {
            // TODO put child process to foreground
            // how to wait for child process
        }
    }
    return 0;
}

void _add_path(char **subarg)
{
    char *prog = subarg[0];
    char *slash_pos = strchr(prog, '/');                               // strrchr(target,key): find the first key and return the pointer or NULL
    char *prog_fullpath = (char *)malloc(CMD_BUFF_MAX * sizeof(char)); // construct program's full path
    if (slash_pos == NULL)                                             // only with base name
    {
        strcpy(prog_fullpath, "/usr/bin/");
    }
    strcat(prog_fullpath, prog);
    subarg[0] = prog_fullpath;
}

char **_copy_cmd(int subargc, char **argv)
{
    /*
    return a copy of the command line arguments
    */
    char **subargv = malloc(sizeof(char *) * (subargc + 1)); // +1 to add a null to signal the end
    for (int count = 0; count < subargc; count++)
    {
        const int arglen = strlen(argv[count]);
        char *arg = (char *)malloc(arglen + 1); // +1 for the \0
        strcpy(arg, argv[count]);
        subargv[count] = arg;
    }
    subargv[subargc] = NULL;
    return subargv;
}

int _split_pipe(char *command, char **cmdv)
{
    // input: {"cmd1", "arg1" ,"|", "cmd2", "arg2", "|", "cmd3", "arg3", "|" ...
    // output: { {"cmd1", "arg1"}, {"cmd2", "arg2"}, {"cmd3", "arg3"}, ..., NULL}
    int subcmd = 0;
    int num_pipes = 0;
    if (strstr(command, "|"))
    {
        char *cmd = strtok(command, "|"); // split by |
        while (cmd)
        {
            cmdv[subcmd] = cmd; //{prog_path, prog_name, arg1, arg2, ..., NULL}
            subcmd++;
            cmd = strtok(NULL, "|");
            if (cmd)
                num_pipes++;
        };
        // printf("# of cmd: %d; # of pipes: %d\n", subcmd, num_pipes);
        if ((num_pipes == 0) | (subcmd != num_pipes + 1))
        {
            invalid_cmd("Error: invalid command\n");
            return -1;
        }
    }
    else
    {
        cmdv[subcmd] = command;
        subcmd++;
    }
    cmdv[subcmd] = NULL;
    return subcmd;
}

void _split_cmd(char **cmdv, char ***cmdvv)
{
    /*
    input: cmdv: {"ls -l","cat"...NULL}
    output: cmdvv: { {"ls", "-l"}, {"cat"},...NULL}
    */
    char **subcmdv;
    char *subcmd;
    for (int i = 0; cmdv[i]; i++)
    {
        int subcmdc = 0;
        // assume a cmd has no more than 10 arg
        subcmdv = malloc(sizeof(char *) * (ARG_MAX));
        if (!strstr(cmdv[i], " ")) // no args
        {
            subcmdv[subcmdc] = cmdv[i];
            subcmdc++;
        }
        else
        {
            // has args
            subcmd = strtok(cmdv[i], " ");
            while (subcmd)
            {
                subcmdv[subcmdc] = subcmd;
                subcmdc++;
                subcmd = strtok(NULL, " ");
            }
        }
        subcmdv[subcmdc] = NULL;
        cmdvv[i] = subcmdv;
    }
}

void _redir_io(char **argv)
{
    int in_redirect = 0;
    int out_redirect = 0;
    int i = 0;
    while (argv[i] != NULL)
    {
        if (strstr(argv[i], "<"))
            in_redirect = i;
        else if (strstr(argv[i], ">") || strstr(argv[i], ">>"))
            out_redirect = i;
        i++;
    };
    // handle input redirect
    if (in_redirect)
        _io_redir_handler(argv, in_redirect, 0);
    // handle output  redirect
    if (out_redirect)
        _io_redir_handler(argv, out_redirect, 1);
    if (in_redirect || out_redirect)
    {
        int arrow_pos = (MIN(in_redirect, out_redirect) != 0 ? MIN(in_redirect, out_redirect) : MAX(in_redirect, out_redirect));
        // "some_cmd < ifile > ofile" can be seen as only exectuing some_cmd with its process fd changed to i/ofile
        // ignore the input after first arrow_pos
        argv[arrow_pos] = NULL;
    }
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
            if (fd < 0) // check if file exists
                invalid_cmd("Error: invalid file\n");
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

int my_system(char *command)
{
    char *cmdv[100];
    char **cmdvv[100];
    int childpids[100];
    int num_p;
    // determine num of processes and split by pipe
    // return -1 if pipe command is illegal
    if ((num_p = _split_pipe(command, cmdv)) == -1)
        return -1;
    _split_cmd(cmdv, cmdvv);
    // for (int i = 0; i < num_p; i++)
    // {
    //     printf("cmdv[%d]: %s\n", i, cmdv[i]);
    // }

    // for (int i = 0; cmdvv[i] != NULL; i++)
    // {
    //     printf("argvv[%d]:", i);
    //     for (int j = 0; cmdvv[i][j] != NULL; j++)
    //         printf("%s ", cmdvv[i][j]);
    //     printf("\n");
    // }
    // printf("num_p: %d\n", num_p);
    //  for loop create child processes
    int all_fildes[2 * (num_p - 1)];
    for (int i = 0; i < num_p - 1; i++)
    {
        if (pipe(all_fildes + i * 2) == -1)
        {
            invalid_cmd("Error: creating pipe\n");
            exit(-1);
        }
    }
    for (int i = 0; i < num_p; i++)
    {
        if ((childpids[i] = fork()) < 0) // child processes
        {
            // fork failed (this shouldn't happen)
            invalid_cmd("Error: fork failed\n");
            exit(1);
        }
        else if (childpids[i] == 0) // child processes
        {
            // handle pipe redirection
            if (num_p > 1)
            {
                // pro0 | pro1 | pro2 | pro3
                if (i > 0) // every process execept the first has stdin redirection
                {
                    dup2(all_fildes[2 * (i - 1)], 0); // change stdin to pipe output
                }
                if (i < num_p - 1) // every process execept the last has stdout redirection
                {
                    dup2(all_fildes[2 * i + 1], 1); // change stdout to pipe output
                }
                for (int i = 0; i < num_p - 1; i++)
                {
                    close(all_fildes[2 * i]);
                    close(all_fildes[2 * i + 1]);
                }
            }
            char **subarg = cmdvv[i];
            //  printf("[son] pid %d from [parent] pid %d\n", getpid(), getppid());
            _add_path(subarg);
            _redir_io(subarg);
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            execv(subarg[0], subarg);
            invalid_cmd("Error: invalid program\n");
            exit(1);
        }
    }
    // parent waits for the children process
    for (int i = 0; i < num_p - 1; i++)
    {
        // printf("pipe%d: rdes=%d and wdes=%d\n", i, all_fildes[2 * i], all_fildes[2 * i + 1]);
        close(all_fildes[2 * i]);
        close(all_fildes[2 * i + 1]);
    }
    int p_status;
    for (int i = 0; i < num_p; i++)
    {
        // printf("Process %d\n", childpids[i]);
        // TASK1: read the status and handle STOP child process
        waitpid(childpids[i], &p_status, WUNTRACED);
        if (WIFSTOPPED(p_status))
        {
            if (!ALL_JOBS->_head_proc)
            {
                ALL_JOBS->_head_proc = malloc(sizeof(struct process));
                ALL_JOBS->_tail_proc = ALL_JOBS->_head_proc;
            }
            else
            {
                ALL_JOBS->_tail_proc->_next = malloc(sizeof(struct process));
                ALL_JOBS->_tail_proc = ALL_JOBS->_tail_proc->_next;
            }
            ALL_JOBS->_tail_proc->_cmd = (char *)malloc(CMD_BUFF_MAX * sizeof(char));
            strcpy(ALL_JOBS->_tail_proc->_cmd, command);
            ALL_JOBS->_tail_proc->_pid = childpids[i];
            ALL_JOBS->_tail_proc->_next = NULL;
            printf("Child Process %d is stopped\n", childpids[i]);
        }
    }
    return 0;
}