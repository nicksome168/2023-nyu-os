#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include "mystructs.c"
#include "helper.c"

void read_disk(char *disk);
void init_all_areas(unsigned char *addr);
void list_fs_info();
void list_dir_entries(int clusterNum);
short trav_entries(DirEntry *dir);
short trav_dir(DirEntry *dir, int clusNum);
DirEntry *get_dir_entry(int clusNum);
DirEntry *get_next_dir(DirEntry *dir);
int get_next_clus(int clusNum);
void recover_file(char *fileName);
short trav_dir_recover(DirEntry *dir, int clusNum, char *recoverFileName, DirEntry *cands[]);
int find_del_file_cand(DirEntry *dir, char *recoverFileName, DirEntry *cands[], short candNum);
int match_del_filename(char *delFileName, char *targFileName);
void recover(DirEntry *dirs[], char *fileName);

BootEntry *bootEntry;
unsigned char *diskAddr;
int endOfRsvd;
int endOfFAT;
int entriesPerClus;
int clusSize;
unsigned char *dataArea;
int *FAT;
/*
2 hrs: milestone 1&2
2 hrs: milestone 3

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
    read_disk(disk);
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
                recover_file(optarg);
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

void read_disk(char *disk)
{
    unsigned char *addr;
    int fd;
    struct stat sb;
    if ((fd = open(disk, O_RDWR)) == -1)
        handle_error("open failed");
    if (fstat(fd, &sb) == -1)
        handle_error("fstat failed");
    if ((addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        handle_error("mmap failed");
    bootEntry = (BootEntry *)addr;
    endOfRsvd = bootEntry->BPB_RsvdSecCnt * bootEntry->BPB_BytsPerSec;
    endOfFAT = endOfRsvd + bootEntry->BPB_FATSz32 * bootEntry->BPB_NumFATs * bootEntry->BPB_BytsPerSec;
    dataArea = addr + endOfFAT;
    entriesPerClus = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus / sizeof(DirEntry);
    clusSize = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;
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
short trav_entries(DirEntry *dir)
{
    short count = 0;
    while (dir->DIR_Name[0] != 0x00 && entriesPerClus > count)
    {
        char fileName[14];
        if (dir->DIR_Name[0] == 0xE5)
        {
            dir++;
            continue;
        }
        get_name(dir->DIR_Name, fileName);
        if (dir->DIR_Attr == 0x10)
            printf("%s/ (starting cluster = %d)\n", fileName, dir->DIR_FstClusLO);
        else
        {
            printf("%s (size = %d", fileName, dir->DIR_FileSize);
            if (dir->DIR_FileSize == 0)
                printf(")\n");
            else
                printf(", starting cluster = %d)\n", dir->DIR_FstClusLO);
        }
        count++;
        dir++;
    }
    return count;
}

short trav_dir(DirEntry *dir, int clusNum)
{
    short entriesCount = 0;
    while (dir)
    {
        entriesCount += trav_entries(dir);
        clusNum = get_next_clus(clusNum);
        if (clusNum == -1)
            break;
        dir = get_dir_entry(clusNum);
    }
    return entriesCount;
}

void list_dir_entries(int clusNum)
{
    DirEntry *rootDir = get_dir_entry(clusNum);
    short entriesCount = trav_dir(rootDir, clusNum);
    printf("Total number of entries = %d\n", entriesCount);
}

int get_next_clus(int clusNum)
{
    if (FAT[clusNum] >= 0x0ffffff8)
        return -1;
    return FAT[clusNum];
}

void recover_file(char *fileName)
{
    int clusNum = (int)bootEntry->BPB_RootClus;
    DirEntry *rootDir = get_dir_entry(clusNum);
    DirEntry *cands[100];
    short candNum = trav_dir_recover(rootDir, clusNum, fileName, cands);
    if (candNum == 1)
    {
        recover(cands, fileName);
        printf("%s: successfully recovered\n", fileName);
    }
    else if (candNum == 0)
        printf("%s: file not found\n", fileName);
    else
    {
        printf("%s: multiple candidates found\n", fileName);
    }
}

short trav_dir_recover(DirEntry *dir, int clusNum, char *recoverFileName, DirEntry *cands[])
{
    short candNum = 0;
    while (dir)
    {
        candNum = find_del_file_cand(dir, recoverFileName, cands, candNum);
        if ((clusNum = get_next_clus(clusNum)) == -1)
            break;
        dir = get_dir_entry(clusNum);
    }
    return candNum;
}

int find_del_file_cand(DirEntry *dir, char *recoverFileName, DirEntry *cands[], short candNum)
{
    char delFileName[13];
    short count = 0;
    while (dir->DIR_Name[0] != 0x00 && entriesPerClus > count)
    {
        get_name(dir->DIR_Name, delFileName);
        if ((delFileName[0] == 0xE5) && match_del_filename(delFileName, recoverFileName))
        {
            cands[candNum] = dir;
            candNum++;
        }
        count++;
        dir++;
    }
    return candNum;
}

void recover(DirEntry *dirs[], char *fileName)
{
    DirEntry *dir = dirs[0];
    dir->DIR_Name[0] = fileName[0];
    int clusCount = dir->DIR_FileSize / clusSize;
    if (dir->DIR_FileSize % clusSize != 0)
        clusCount++;
    int clusNum = dir->DIR_FstClusHI << 16 | dir->DIR_FstClusLO;
    for (int i = 0; i < clusCount - 1; i++)
    {
        FAT[clusNum] = clusNum + 1;
        clusNum++;
    }
    FAT[clusNum] = 0x0ffffff8;
}
