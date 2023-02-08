#include "cmdreader.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// how to use getline: https://c-for-dummies.com/blog/?p=1112s
// how to use getcwd: https://iq.opengenus.org/chdir-fchdir-getcwd-in-c/
// how to continuously getchar() from a line: https://stackoverflow.com/questions/33947693/why-does-getchar-continue-to-take-characters-from-a-line-of-input-instead-of-a
// how to copy chars: https://stackoverflow.com/questions/6205195/given-a-starting-and-ending-indices-how-can-i-copy-part-of-a-string-in-c
// include wait library: https://stackoverflow.com/questions/41884685/implicit-declaration-of-function-wait
// How to use execv with a generated path in C?: https://stackoverflow.com/questions/52240612/how-to-use-execv-with-a-generated-path-in-c
// How to make parent wait for all child processes to finish?: https://stackoverflow.com/questions/19461744/how-to-make-parent-wait-for-all-child-processes-to-finish
// How to use strtok: https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string

void get_curdir(char *abs_path, char *relat_path);
int main()
{
    size_t PATH_MAX = 1024;
    size_t CMD_BUFF_MAX = 1000;
    size_t characters = 0;
    char *abs_path = (char *)malloc(PATH_MAX * sizeof(char));
    char *relat_path = (char *)malloc(PATH_MAX * sizeof(char));
    char *cmd_buffer = (char *)malloc(CMD_BUFF_MAX * sizeof(char));
    while (1)
    {
        getcwd(abs_path, PATH_MAX * sizeof(char));
        get_curdir(abs_path, relat_path);

        // print prompt
        printf("[nyush %s]$ ", relat_path);
        fflush(stdout);
        characters = getline(&cmd_buffer, &CMD_BUFF_MAX, stdin);
        cmd_buffer[characters - 1] = '\0'; // remote newline

        if (strcmp(cmd_buffer, "exit") == 0)
        {
            break;
        }
        else if (characters == 1)
        {
            continue;
        }
        else
        {
            int pid = fork();
            if (pid < 0)
            {
                // fork failed (this shouldn't happen)
                fprintf(stderr, "Error: fork failed\n");
                exit(1);
            }
            else if (pid == 0)
            {
                // determin which command it is
                char *slash_pos = strchr(cmd_buffer, '/'); // strrchr(target,key): find the first key and return the pointer or NULL
                char *prog_path = (char *)malloc(PATH_MAX * sizeof(char));
                char *arg = strtok(cmd_buffer, " ");
                if (slash_pos == NULL)
                {
                    // only with base name
                    strcat(prog_path, "/usr/bin/");
                    strcat(prog_path, arg);
                }
                else
                {
                    // relative path or absolute path: e.g., /usr/bin/ls
                    strcat(prog_path, arg);
                }
                char *argv[] = {prog_path}; //{prog_path, prog_name, arg1, arg2, ..., NULL}
                int argc = 1;
                // add arg to argv
                while ((arg = strtok(NULL, " ")))
                {
                    argv[argc] = arg;
                    argc++;
                }
                argv[argc] = NULL;
                // for (int i = 0; argv[i] != NULL; i++)
                //     printf("argv %d: %s\n", i, argv[i]);
                execvp(argv[0], argv);
                fprintf(stderr, "Error: invalid program\n");
                fflush(stdout);
                exit(1);
            }
            else
            {
                // parent waits for the children process
                wait(NULL);
            }
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
