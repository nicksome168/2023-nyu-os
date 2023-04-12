# HW6

Nick Chao | yc6371

## q1

The output is as follows:

```bash
BobvT2
00103254
```

The different output is attributed to the byte-alignment. Since by default the compiler will align the memory of the struct varibales to 4-byte boundary, `0x76` will be interpreted as part of the `v1`. Thus, the output is `BobvT2` instead of `Bob`. For the second output, the `v2` is aligned to 4-byte boundary, so the last `0` is interpreted as part of the `v2`, and since little endian is used for byte ordering, `v2` is interpreted in the reversed order as `0x00`, `0x10`, `0x32`, `0x54`. Thus, the output is `00103254` not `76543210`, which would be the case if it's in big endian order.

## q2

`mmap()` could be an alternative way to access random memory without having `lseek()` system calls. `mmap()` is more suitable for working on small files or a small portion of large files because then the whole/part of the file can be prefetched in page cache.As for `lseek()`, it is more suitable for working on large files, specifically when the working area is too large for a page. In this case, `lseek()` can be used to move the file pointer to the desired location and then read/write the data.

### Reference
<https://stackoverflow.com/questions/19653618/mmap-will-the-mapped-file-be-loaded-into-memory-immediately>

## q3

For linked-list-based implementation, a corrupted data block will likely result in the corruption of the whole file. For example, if the data block is corrupted, the `next` pointer will point to a random address, which will cause the program to read the data from a random location. For table-based implementation, this is less likely to happen because data block pointers are centralized in the table. If the data block is corrupted, the program will still be able to read other data blocks from the table.
