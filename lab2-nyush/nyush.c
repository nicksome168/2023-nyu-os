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
// how to continuously getchar() from a line: https://stackoverflow.com/questions/33947693/why-does-getchar-continue-to-take-chars-from-a-line-of-input-instead-of-a
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

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct process
{
    struct process *_next;
    int _pid;
    char *_cmd;
} process;
typedef struct jobs
{
    struct process *_head_proc; // the head of the process list
    struct process *_tail_proc; // the tail of the process list
} jobs;

//----------------------------------------------------------------------
//    Global variables
//----------------------------------------------------------------------
struct jobs _ALL_JOBS = {._head_proc = NULL, ._tail_proc = NULL};
struct jobs *ALL_JOBS = &_ALL_JOBS;
const size_t CMD_BUFF_MAX = 1000;
const size_t ARG_MAX = 50; // assume no more than ARG_MAX arguments
size_t CMD_SIZE = CMD_BUFF_MAX * sizeof(char);

/*prompt-related*/
void get_curdir(char *cur_dir);
void handle_invalid_cmd(char *err_msg);
/*handle io*/
int help_ioredir(char **argv, int arrow_pos, int io_flag);
int handle_ioredir(char **argv);
/*handle builtin commands*/
int check_is_builtin_cmd(char *command);
int handle_builtin_cmd(char *command);
/*handle commands*/
int handle_system(char *command);
int handle_pipe_split(char *command, char **cmdv); // split the whole command by pipe
void handle_cmd_split(char **cmdv, char ***cmdvv); // split within command by space
char *handle_prog_locat(char **subarg);            // add path to program
/*handle processes*/
void handle_proc_cont(int fg_idx);        // continue child process
void handle_proc_stp(int pid, char *cmd); // stop child process

int main()
{
    int chars;
    char *cur_dir = (char *)malloc(CMD_SIZE);
    char *cmd = (char *)malloc(CMD_SIZE);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    while (1)
    {
        get_curdir(cur_dir);
        printf("[nyush %s]$ ", cur_dir);
        fflush(stdout);
        chars = getline(&cmd, &CMD_SIZE, stdin);
        cmd[chars - 1] = '\0'; // remove newline
        if (chars == -1)       // ctrl+d
        {
            break;
        }
        else if (chars == 1) // enter
        {
            continue;
        }
        else if (check_is_builtin_cmd(cmd))
        {
            if (handle_builtin_cmd(cmd) == -1)
            {
                if (ALL_JOBS->_head_proc != NULL)
                {
                    handle_invalid_cmd("Error: there are suspended jobs\n");
                    continue;
                }
                break;
            }
        }
        else
        {
            handle_system(cmd);
        }
    }
    /*clean up*/
    struct process *ptr = ALL_JOBS->_head_proc;
    struct process *nxt_ptr;
    while (ptr)
    {
        free(ptr->_cmd);
        nxt_ptr = ptr->_next;
        free(ptr);
        ptr = nxt_ptr;
    }
    free(cur_dir);
    free(cmd);
}

void get_curdir(char *cur_dir)
{
    getcwd(cur_dir, CMD_SIZE);
    if (strcmp(cur_dir, "/") != 0)
    {
        char *lslash = strrchr(cur_dir, '/'); // e.g., "/2250"
        strcpy(cur_dir, lslash + 1);
    }
}

void handle_invalid_cmd(char *err_msg)
{
    fprintf(stderr, err_msg);
    fflush(stdout);
}

int check_is_builtin_cmd(char *command)
{
    if (strstr(command, "cd") || strstr(command, "jobs") || strstr(command, "fg") || strstr(command, "exit"))
        return 1;
    return 0;
}

int handle_builtin_cmd(char *command)
{
    strtok(command, " ");          // return cmd itself
    char *arg = strtok(NULL, " "); // return first arg
    if (strstr(command, "jobs") || strstr(command, "exit"))
    {
        // should take no argument
        if (arg != NULL)
        {
            handle_invalid_cmd("Error: invalid command\n");
            return 0;
        }
        if (strstr(command, "exit"))
        {
            return -1;
        }
        else if (strstr(command, "jobs")) // jobs cmd
        {
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
    else if (strstr(command, "cd") || strstr(command, "fg")) // should not take 0 or 2+ arguments
    {
        if (arg == NULL || strtok(NULL, " ") != NULL)
        {
            handle_invalid_cmd("Error: invalid command\n");
            return 0;
        }
        if (strstr(command, "cd"))
        {
            if (chdir(arg) < 0)
            {
                handle_invalid_cmd("Error: invalid directory\n");
            }
        }
        else if (strstr(command, "fg")) // fg cmd
        {
            int fg_idx = atoi(arg) - 1; // accomodate 0-indexed process list
            handle_proc_cont(fg_idx);
        }
    }
    return 0;
}

char *handle_prog_locat(char **subarg)
{
    char *prog = subarg[0];
    char *slash_pos = strchr(prog, '/');            // strrchr(target,key): find the first key and return the pointer or NULL
    char *prog_fullpath = (char *)malloc(CMD_SIZE); // construct program's full path
    if (slash_pos == NULL)                          // only with base name
    {
        strcpy(prog_fullpath, "/usr/bin/");
    }
    strcat(prog_fullpath, prog);
    return prog_fullpath;
}

int handle_pipe_split(char *command, char **cmdv)
{
    // input: {"cmd1", "arg1" ,"|", "cmd2", "arg2", "|", "cmd3", "arg3", "|" ...
    // output: { {"cmd1", "arg1"}, {"cmd2", "arg2"}, {"cmd3", "arg3"}, ..., NULL}
    int cmd_c = 0;
    int num_pipes = 0;
    char *copy_command = (char *)malloc(CMD_SIZE);
    strcpy(copy_command, command);
    char *cmd = strstr(copy_command, "|");
    if (cmd != NULL)
    {
        cmd = strtok(copy_command, "|");
        while (cmd)
        {
            cmdv[cmd_c] = cmd;
            cmd_c++;
            cmd = strtok(NULL, "|");
            if (cmd != NULL)
                num_pipes++;
        };
        if ((num_pipes == 0) | (cmd_c != num_pipes + 1)) // | cat
        {
            handle_invalid_cmd("Error: invalid command\n");
            return -1;
        }
    }
    else
    {
        cmdv[cmd_c] = copy_command;
        cmd_c++;
    }
    cmdv[cmd_c] = NULL;
    return cmd_c;
}

void handle_cmd_split(char **cmdv, char ***cmdvv)
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
        subcmdv = malloc(sizeof(char *) * (ARG_MAX));
        if (!strstr(cmdv[i], " ")) // no args
        {
            subcmdv[subcmdc] = cmdv[i];
            subcmdc++;
        }
        else // has args
        {
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

int handle_ioredir(char **argv)
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
    if (in_redirect)
        if (help_ioredir(argv, in_redirect, 0) == -1)
            return -1;
    if (out_redirect)
        if (help_ioredir(argv, out_redirect, 1) == -1)
            return -1;
    if (in_redirect || out_redirect)
    {
        int arrow_pos = (MIN(in_redirect, out_redirect) != 0 ? MIN(in_redirect, out_redirect) : MAX(in_redirect, out_redirect));
        // "some_cmd < ifile > ofile" can be seen as only exectuing some_cmd with its process fd changed to i/ofile
        // ignore the input after first arrow_pos
        argv[arrow_pos] = NULL;
    }
    return 0;
}
int help_ioredir(char **argv, int arrow_pos, int io_flag)
{
    char *file;
    if (argv[arrow_pos + 1] != NULL)
    {
        file = malloc(strlen(argv[arrow_pos + 1]) * sizeof(char));
        strcpy(file, argv[arrow_pos + 1]);
        int fd;
        if (io_flag) // output
        {
            fd = open(file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
            if (fd < 0)
            {
                handle_invalid_cmd("Error: invalid file\n");
                return -1;
            }
            dup2(fd, 1);
        }
        else // input
        {
            fd = open(file, O_RDONLY);
            if (fd < 0)
            {
                handle_invalid_cmd("Error: invalid file\n");
                return -1;
            }
            dup2(fd, 0);
        }
        close(fd);
        free(file);
        return 0;
    }
    else // no file specified
    {
        handle_invalid_cmd("Error: invalid command\n");
        return -1;
    }
}

int handle_system(char *command)
{
    char *cmdv[100];
    char **cmdvv[100];
    int childpids[100];
    int num_p;
    if ((num_p = handle_pipe_split(command, cmdv)) == -1) // determine num of processes and split by pipe
        return -1;                                        // return -1 if pipe command is illegal
    handle_cmd_split(cmdv, cmdvv);
    /*create file descriptor*/
    int all_fildes[2 * (num_p - 1)];
    for (int i = 0; i < num_p - 1; i++)
    {
        if (pipe(all_fildes + i * 2) == -1)
        {
            handle_invalid_cmd("Error: creating pipe\n");
            exit(-1);
        }
    }
    /*create child process*/
    for (int i = 0; i < num_p; i++)
    {
        if ((childpids[i] = fork()) < 0) // child processes
        {
            handle_invalid_cmd("Error: fork failed\n");
            exit(1);
        }
        else if (childpids[i] == 0) // child processes
        {
            if (num_p > 1) // handle pipe redirection
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
                for (int i = 0; i < num_p - 1; i++) // close all file descriptors
                {
                    close(all_fildes[2 * i]);
                    close(all_fildes[2 * i + 1]);
                }
            }
            char **subarg = cmdvv[i];
            char *prog_path = handle_prog_locat(subarg);
            if (handle_ioredir(subarg) < 0)
                exit(1);
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            execv(prog_path, subarg);
            handle_invalid_cmd("Error: invalid program\n");
            exit(1);
        }
    }
    for (int i = 0; i < num_p - 1; i++)
    {
        close(all_fildes[2 * i]);
        close(all_fildes[2 * i + 1]);
    }
    int p_status;
    for (int i = 0; i < num_p; i++)
    {
        waitpid(childpids[i], &p_status, WUNTRACED);
        if (WIFSTOPPED(p_status))
            handle_proc_stp(childpids[i], command);
    }
    return 0;
}

void handle_proc_stp(int pid, char *command)
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
    ALL_JOBS->_tail_proc->_cmd = (char *)malloc(CMD_SIZE);
    strcpy(ALL_JOBS->_tail_proc->_cmd, command);
    ALL_JOBS->_tail_proc->_pid = pid;
    ALL_JOBS->_tail_proc->_next = NULL;
}
void handle_proc_cont(int fg_idx)
{
    int p_status;
    struct process *ptr = ALL_JOBS->_head_proc;
    struct process *pre_ptr = NULL;
    while (fg_idx > 0 && ptr && (ptr != ALL_JOBS->_tail_proc))
    {
        fg_idx--;
        pre_ptr = ptr;
        ptr = ptr->_next;
    }
    if (ptr && fg_idx == 0)
    {
        if (ptr == ALL_JOBS->_tail_proc) // located at the tail
        {
            ALL_JOBS->_tail_proc = pre_ptr;
        }
        if (pre_ptr == NULL) // located at the head
        {
            ALL_JOBS->_head_proc = ptr->_next;
        }
        else // located in the middle
        {
            pre_ptr->_next = ptr->_next;
        }
        int pid = ptr->_pid;
        if (kill(pid, SIGCONT) == 0)
        {
            waitpid(pid, &p_status, WUNTRACED);
            if (WIFSTOPPED(p_status))
                handle_proc_stp(pid, ptr->_cmd);
            free(ptr->_cmd);
            free(ptr);
        }
        else
        {
            handle_invalid_cmd("Error: continue job failed\n");
            exit(1);
        }
    }
    else
    {
        handle_invalid_cmd("Error: invalid job\n");
    }
}