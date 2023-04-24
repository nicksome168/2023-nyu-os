# HW7

Nick Chao | yc6371

## q1 For each line marked with (a) through (e), will it cause a segmentation fault? Why?

(c) causes segmentation fault because pointer `c` is set to a static memory location, which is not allowed to be modified.

## q2 Name one advantage of hard links over symbolic links and one advantage of symbolic links over hard links

hard link is linked to the same inode, meaning there won't be dangling pointer issue if the original link is deleted. Symbolic link has more flexibility as it only stores the name of the original link, so it can be used across different file systems.

## q3 For an external USB drive attached to a computer, which is more suitable: a write-through cache or a write-back cache?

Write-through cache would be more suitable because in most usecases USB drivers are not removed safely. If the cache is write-back, the data would be lost. Thus it is better to write the data to USB drivers immediately.

## q4 What is the difference between a physical address and a virtual address?

Physical address actually refers to a physical location on the memory, while virtual address is a logical address that is mapped to a physical address. Every program has its own distinct virtual address to protect programs from crashing each other.

### Reference

<https://stackoverflow.com/questions/3243610/difference-between-physical-addressing-and-virtual-addressing-concept>
