#include "cmdreader.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PATH_MAX 1024
#define CMD_MAX 4096
int RUN = 1;
// how to use getcwd: https://iq.opengenus.org/chdir-fchdir-getcwd-in-c/
// how to continuously getchar() from a line: https://stackoverflow.com/questions/33947693/why-does-getchar-continue-to-take-characters-from-a-line-of-input-instead-of-a
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
        printf("[nyush %s]$ ", relat_path);
        buf_end = 0;
        // catch ctrl+d and free the memory
        while ((c = getchar()) != EOF && c != '\n' && buf_end < CMD_MAX - 1)
        {
            // get a line of input
            cmd_buffer[buf_end] = c;
            buf_end++;
        }
        cmd_buffer[buf_end] = '\0';
        printf("%s\n", cmd_buffer);
        fflush(stdout);
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