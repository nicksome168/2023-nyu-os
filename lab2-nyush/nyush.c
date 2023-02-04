#include "cmdreader.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PATH_MAX 1024
#define CMD_MAX 4096
int RUN = 1;
// how to use getcwd: https://iq.opengenus.org/chdir-fchdir-getcwd-in-c/
// how to continuously getchar() from a line: https://stackoverflow.com/questions/33947693/why-does-getchar-continue-to-take-characters-from-a-line-of-input-instead-of-a
// how to copy chars: https://stackoverflow.com/questions/6205195/given-a-starting-and-ending-indices-how-can-i-copy-part-of-a-string-in-c
// include wait library: https://stackoverflow.com/questions/41884685/implicit-declaration-of-function-wait
// How to use execv with a generated path in C?: https://stackoverflow.com/questions/52240612/how-to-use-execv-with-a-generated-path-in-c
// How to make parent wait for all child processes to finish?: https://stackoverflow.com/questions/19461744/how-to-make-parent-wait-for-all-child-processes-to-finish
void get_curdir(char *abs_path, char *relat_path);
int main()
{
    int c = 0;
    int buf_end = 0;
    char *abs_path = (char *)malloc(PATH_MAX * sizeof(char));
    char *relat_path = (char *)malloc(PATH_MAX * sizeof(char));
    char *cmd_buffer = (char *)malloc(CMD_MAX * sizeof(char));
    while (c != EOF)
    {
        getcwd(abs_path, PATH_MAX * sizeof(char));
        get_curdir(abs_path, relat_path);
        // print prompt
        printf("[nyush %s]$ ", relat_path);
        buf_end = 0;
        // read the command
        while ((c = getchar()) != EOF && c != '\n' && buf_end < CMD_MAX - 1)
        {
            // get a line of input
            cmd_buffer[buf_end] = c;
            buf_end++;
        }
        if (c != EOF)
        {
            cmd_buffer[buf_end] = '\0';
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
                char *slash_pos = strrchr(cmd_buffer, '/'); // strrchr(target,key): find the first key and return the pointer or NULL
                char *argv[] = {NULL};
                char *path = (char *)malloc(PATH_MAX * sizeof(char));
                if (slash_pos == cmd_buffer)
                {
                    //  absolute path
                    strcat(path, cmd_buffer);
                }
                else if (slash_pos != NULL && slash_pos != cmd_buffer)
                {
                    // relative path
                    strcat(path, "./");
                    strcat(path, cmd_buffer);
                }
                else
                {
                    // only with base name
                    strcat(path, "/usr/bin/");
                    strcat(path, cmd_buffer);
                    fflush(stdout);
                }
                printf("full command: %s\n", path);
                execv(path, argv);
                fprintf(stderr, "Error: invalid program\n");
            }
            else
            {
                // parent waits for the children process
                while (waitpid(-1, NULL, 0) > 0)
                    ;
            }
        }
    }
    printf("\n");
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