# HW1
Nick Chao | yc6371
## q1
(1) Can you explain what’s happening here?
Alice assigned the address of a local variable to a returned pointer. Local variable only lives within its function scope, so once the function finishes executing the local variable is freed. Thus, the memeory the returned pointer is pointing to no longer holds the value.

(2) Alice insists that add() should return a pointer. Can you propose a way to fix it?
We assign a memory using malloc to the returned pointer, copy the sum by value and return the pointer. Since malloc is allocating memories from heap, it won't be freed as the function finishes. At the end, we have to manually free the allocated memory.
```c
#include <stdio.h>
int *add(int a, int b)
{
    int c = a + b;
    int *d = malloc(sizeof(int));
    *d = c;
    return d;
}
int main()
{
    int *result = add(1, 2);
    printf("result = %d\n", *result);
    printf("result = %d\n", *result);
    free(result);
}
```

## q2
### Can you explain to Bob what each command does? What would happen if Bob executes these commands in order?
1. take 'echo cat' as the input of `cat` command
2. write 'echo cat' to a file called 'cat'
3. append 'echo cat' to the `cat` file
4. execute `echo cat` and since echo doesn't read its stdin ([ref: stackoverflow](https://stackoverflow.com/questions/35116699/piping-not-working-with-echo-command)), the first echo outputs nothing, and nothing is passed as input of `cat` command.
5. rewire the stdin stream of `cat` command from the file `cat`
```bash
1. echo 'echo cat' | cat    #'echo cat' in prompt
2. echo 'echo cat' > cat    #nothing in prompt. Create a file called 'cat' containing 'echo cat'
3. echo 'echo cat' >> cat   #nothing in prompt. Append with another line of 'echo cat' in the 'cat' file
4. echo `echo cat` | cat    #'cat' in prompt. 
5. cat < cat                #Two lines of 'echo cat' in prompt.
```

## q3
### What is the difference between kernel and user mode? Explain how having two distinct modes aids in designing an operating system.
Previledged instructions can only be executed under kernal mode not under user mode. The purpose is to protect operating systems from errent users.
(ref: Operating System Concepts-Wiley, 1.4 Operating-System Operations 25)

## q4
### What is a trap instruction? Explain its use in operating systems.
When users make system calls, the calls will issue a trap instruction to OS to switch from user mode to kernal mode.
(ref: Operating System Concepts-Wiley, 1.4 Operating-System Operations 25)

## q5
### Instructions related to accessing I/O devices are typically privileged instructions, that is, they can be executed in kernel mode but not in user mode. Give a reason why these instructions are privileged.
Because without having I/O devices being priviledged, errent users might access and modify the part of these devices they shouldn't have had access to, causing systems breakdown or crashes.