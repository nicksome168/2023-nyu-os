# HW4

Nick Chao | yc6371

## q1

### 1. Using the program and the comments, describe at a high level what this code is intended to do. What output did Alice expect the program to generate?

Alice is implementing an append-only single-link-list-like data structure.

### 2. What happens when the program is run?

Supposedly the program will create three "boxes" and chain them sequentially in the order they were created. Finally, print them out. However, the resulting output was not same as expected. It should have looked something like as follows, but instead from the second level the output is wrong and it stops at this level.

```bash
#expected
\- id: 37
\- - id: 12
\- - - id: 19

#what we got
\- id: 37
\- - id: -824424144 #random number
```

### 3. What is the error in Alice’s reasoning?

At line 13, Alice reasons that **"Since `inner` is not being modified, we pass in `inner` directly"**. This makes `inner` being passed by value and in turn becomes a local variable, resulting in undefined behavior when she trys to reference it at line 17.

### 4. Change this program to produce the intended output

Change the `inner` parameter for `insert_box()` to pass by reference by passing in the pointer of `inner`.

## q2

### When a computer is being developed, it is usually first simulated by a program that runs one instruction at a time. Even multiprocessors are simulated strictly sequentially like this. Is it possible for a race condition to occur when there are no simultaneous events like this?

Yes, race conditions still can occur becasue the sequence of process execution is still non-deterministic, and context switching can happen at any time.

## q3

### The producer-consumer problem can be extended to a system with multiple producers and consumers that write (or read) to (from) one shared buffer. Assume that each producer and consumer runs in its own process. Will the solution presented in Slide 208 of 2-proc.pdf, using semaphores, work for this system?

Yes, we just need to make the semphore process-shared. According to [man page](https://man7.org/linux/man-pages/man7/sem_overview.7.html), semophore can be process-shared, meaning cross-process sychronization is possible.
