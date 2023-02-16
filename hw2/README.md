# HW2

Nick Chao | yc6371

## q1

(1) Which line(s) prints 13? Why?
First line prints 13, because in `sizeof(a)` a is passed in as an char array of size 13 comprising of the 12 characters and the `\0` terminating the array.

Behind the scence `a[]`, an array of chars, is making a copy from a block of static read-only memory that holds the string "Hello, 2250!". Essentially, `a` is an array of chars.

(2) What do other line(s) print? Why?
All printed 8 because in `sizeof(b)` b is passed in as a pointer and on a 64-bit system like mac pointer size is 8 bytes. The reason `sizeof(b)` is not 13 is because behind the scence `b`, a pointer, is pointing to a block of static read-only memory that holds the string "Hello, 2250!". Essentially, `b` is a pointer.

For `f(a)`, since `a` is also passed in to the function `f()` as a char pointer, the return will also be 8 bytes. Lastly, the same result is from `f(b)` as `b` is passed in as a pointer.

(3) In fact, sizeof() shouldn’t be used to get the length of a string at all. Why?
`sizeof()` only gives us the size of allocated memory associated with the given type or variable. In some cases, it doesn't equal to the length of a string. For example, `a[20] = "Hello, 2250!"`. `sizeof(a)` will return 20.
Plus, most of the time in c string is passed by reference as a pointer, so if we try to gauge the size of a string we would only get the size of a pointer. Plus if
In this case, we should use `strlen()` in the `string.h` library instead.

### References

- [What is the difference between char s[] and char *s?](https://stackoverflow.com/questions/1704407/what-is-the-difference-between-char-s-and-char-s/1704556#1704556)
- [difference between sizeof and strlen in c [duplicate]](https://stackoverflow.com/questions/8590332/difference-between-sizeof-and-strlen-in-c)

## q2

(1) Can you explain to Bob what’s happening here? Can you propose a way to make both “one” and “two” printed?
Becuase stdout is by default line-buffered, when the program executes `execl()` "one" is still buffered and thus was wiped out before being printed. To have it not buffered, Bob has to add "\n" at the end of the line.

(2) Can you propose a way to make all “one”, “two”, and “three” printed?
Bob can use `fork()` to create child process to execute `execl()` and wait for the child process to terminate and print "three" afterwards.

## q3

For each of the following system calls, give a condition that causes it to fail: fork, exec, and wait.

- `fork`: no more pid left, or no more meomry to create a copy from the parent process
- `exec`: executable doesn't exist, or the program `exec` executes has segment
- `wait`: no child process

### References

- [Error:  Failed to fork child process, errno=11, GENTRAN:Server for UNIX (826)](https://www.ibm.com/support/pages/error-%C2%A0failed-fork-child-process-errno11-gentranserver-unix-826#:~:text=The%20two%20main%20reasons%20fork,memory%20could%20be%20a%20factor.%3F)
- [Can UNIX exec return an error?](https://www.quora.com/Can-UNIX-exec-return-an-error)

## q4

To a programmer, a system call looks like any other call to a library function. Is it important that a programmer know which library functions result in system calls? Under what circumstances and why?

Yes, in situations where a programmer wants to reduce the system calls in order to speed up program execution. For example, if a programmer wants to print 1000 "Hello", then it will be useful to know `printf()` is a wrapper of the system call `write()`. Thus, a program printing 10 "Hello"s in a 100-time for loop will execute faster than one printing it one by one.
