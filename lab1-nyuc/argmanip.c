#include "argmanip.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*Referneces
 *
 *  1. malloc usage: 6.5 Self−referential Structures p130, https://en.cppreference.com/
 *  2. va_list, va_start, va_arg, va_end usage: 7.3 Variable−length Argument Lists p141, https://en.cppreference.com/
 */

char **manipulate_args(int argc, const char *const *argv, int (*const manip)(int))
{
    /*
    argc: # of arguments
    argv: Pointer to the first element of an array of argc + 1 pointers. The last element is null
    */
    char **args = malloc(sizeof(char *) * (argc + 1)); // +1 to add a null to signal the end
    for (int count = 0; count <= argc; count++)
    {
        if (count == argc)
            args[count] = NULL;
        else
        {
            const int arglen = strlen(argv[count]);
            char *arg = (char *)malloc(arglen + 1); // +1 for the \0
            if (arg != NULL)
            {
                for (int i = 0; i <= arglen; i++)
                {
                    arg[i] = manip(argv[count][i]);
                }
            }
            args[count] = arg;
        }
    }
    return args;
}

void free_copied_args(char **args, ...)
{
    va_list ap;
    va_start(ap, args);
    while (args != NULL)
    {
        char **curArg = args;
        while (*curArg)
        {
            free(*curArg);
            curArg++;
        }
        free(args);
        args = va_arg(ap, char **);
    }
    va_end(ap);
    return;
}