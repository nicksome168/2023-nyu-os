#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include "mystructs.c"
#include "helper.c"

unsigned char *read_disk(char *disk);
void init_all_areas(unsigned char *addr);
void list_fs_info();
void list_dir_entries(int clusterNum);
short _helper_list_entries(DirEntry *dir);
DirEntry *get_dir_entry(int clusNum);
DirEntry *get_next_dir(DirEntry *dir);
int get_next_clus(int clusNum);

BootEntry *bootEntry;
int endOfRsvd;
int endOfFAT;
int entriesPerClus;
unsigned char *dataArea;
int *FAT;
/*
2 hrs: milestone 1&2

how to get cluster number: https://github.com/vivisect/dissect/blob/master/dissect/formats/fat32.py
*/

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 6)
    {
        print_usage();
        return 0;
    }
    char *disk = argv[1];
    unsigned char *addr = read_disk(disk);
    init_all_areas(addr);
    int ch;
    int select = 0;
    int selectR = 0;
    while ((ch = getopt(argc, argv, "ilr:R:s:")) != -1)
    {
        switch (ch)
        {
        case 'i':
            if (select == 0)
            {
                select = 1;
                list_fs_info();
            }
            break;
        case 'l':
            if (select == 0)
            {
                select = 1;
                list_dir_entries((int)bootEntry->BPB_RootClus);
            }
            break;
        case 'r':
            if (select == 0)
            {
                select = 1;
                selectR = 1;
            }
            break;
        case 'R':
            if (select == 0)
            {
                select = 1;
                selectR = 1;
            }
            break;
        case 's':
            if (selectR == 1)
            {
                //     selectS = 1;
            }
            break;
        default:
            print_usage();
        }
    }

    return 0;
}

unsigned char *read_disk(char *disk)
{
    unsigned char *addr;
    int fd;
    struct stat sb;
    if ((fd = open(disk, O_RDONLY)) == -1)
        handle_error("open failed");
    if (fstat(fd, &sb) == -1)
        handle_error("fstat failed");
    if ((addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
        handle_error("mmap failed");
    return addr;
}

void init_all_areas(unsigned char *addr)
{
    bootEntry = (BootEntry *)addr;
    endOfRsvd = bootEntry->BPB_RsvdSecCnt * bootEntry->BPB_BytsPerSec;
    endOfFAT = endOfRsvd + bootEntry->BPB_FATSz32 * bootEntry->BPB_NumFATs * bootEntry->BPB_BytsPerSec;
    dataArea = addr + endOfFAT;
    entriesPerClus = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus / sizeof(DirEntry);
    FAT = (int *)(addr + endOfRsvd);
}

void list_fs_info()
{
    printf("Number of FATs = %d\n", bootEntry->BPB_NumFATs);
    printf("Number of bytes per sector = %d\n", bootEntry->BPB_BytsPerSec);
    printf("Number of sectors per cluster = %d\n", bootEntry->BPB_SecPerClus);
    printf("Number of reserved sectors = %d\n", bootEntry->BPB_RsvdSecCnt);
}

DirEntry *get_dir_entry(int clusNum)
{
    return (DirEntry *)(dataArea + (clusNum - 2) * bootEntry->BPB_SecPerClus * bootEntry->BPB_BytsPerSec);
}
short _helper_list_entries(DirEntry *dir)
{
    char *fileName = malloc(9);
    char *fileExt = malloc(4);
    short count = 0;
    while (dir->DIR_Name[0] != 0x00 && entriesPerClus > count)
    {
        if (dir->DIR_Name[0] == 0xE5)
        {
            dir++;
            continue;
        }
        get_name(dir->DIR_Name, fileName, fileExt);
        if (dir->DIR_Attr == 0x10)
            printf("%s/ (starting cluster = %d)\n", fileName, dir->DIR_FstClusLO);
        else
        {

            if (strcmp(fileExt, "\0") == 0)
                printf("%s (size = %d", fileName, dir->DIR_FileSize);
            else
                printf("%s.%s (size = %d", fileName, fileExt, dir->DIR_FileSize);

            if (dir->DIR_FileSize == 0)
                printf(")\n");
            else
                printf(", starting cluster = %d)\n", dir->DIR_FstClusLO);
        }
        dir++;
        count++;
    }
    free(fileName);
    free(fileExt);
    return count;
}
void list_dir_entries(int clusNum)
{
    DirEntry *rootDir = get_dir_entry(clusNum);
    short count = 0;
    while (rootDir)
    {
        count += _helper_list_entries(rootDir);
        if ((clusNum = get_next_clus(clusNum)) == -1)
            break;
        rootDir = get_dir_entry(clusNum);
    }
    printf("Total number of entries = %d\n", count);
}

int get_next_clus(int clusNum)
{
    if (FAT[clusNum] >= 0x0ffffff8)
        return -1;
    return FAT[clusNum];
}