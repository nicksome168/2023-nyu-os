# HW3

Nick Chao | yc6371

## q1

(1) Can you explain this issue to Alice and help her correct it?

Since `strncpy(dest, src, n)` copies from `src` to `dest` up to `n` bytes including the null terminator, `src2` is **not null terminated** in this case. `printf()` will have undefined behavior when being passed with non-null terminated strings.

To have correct results, Alica has to increase the arrary size by 1 to count in the null terminator.

### Reference

<https://stackoverflow.com/questions/61250720/problem-on-strncpy-with-source-string-longer-than-destination-array>

## q2

(1) Can you explain to Alice what’s happening here?

Since `arr1` is casted as char array, the memory is read differently. Instead of reading every 4 bytes like an integer array, now it's read byte by byte. If we look at the memory of the `arr1`, we know the first 4 bytes is `01 00 00 00`, which is read as `1` in an integer array. Thus, if now we read it byte by byte, we will get `1 0 0 0`, and since `strncpy()` will terminate the copy if encountered null terminator and padded the dest array with 0s, the resulting `arr2` will be `{1,0,0,0}`, namely 1 byte of `01` followed by 15 bytes of padding 0s.

(2) Instead of strncpy(), what function should be used to copy an array?

Alice can use a for loop or `memcpy()`.

```c
for (int i = 0; i <4; i++)
    arr2[i] = arr1[i];
//or
memcpy(arr2, arr1, 4 * sizeof(int));
```

### Reference

<https://stackoverflow.com/questions/7844049/how-are-c-arrays-represented-in-memory>

## q3

When an interrupt or a system call transfers control to the operating system, a kernel stack area separate from the stack of the interrupted process is generally used. Why?

The isolation of kernal stack is necessary because if other processes crash, the kernal will be intact. Also, it will keep the kernal stack from malicious usage.

### Reference

<https://www.baeldung.com/linux/kernel-stack-and-user-space-stack>

## q4

In theory, with three states, there could be six transitions, two out of each state. However, only four transitions are shown. Are there any circumstances in which either or both of the missing transitions might occur?

It is not possible to go from `Ready` to `Block` state because in `Ready` state the process is not executed and thus cannot initiate I/O operations.

It is possible to go from `Block` to `Running` state, but there are many assumptions in order to have this state transition. For example, assume that there is only one process running or there exists no dual mode.
