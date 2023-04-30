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
short trav_entries(DirEntry *dir, int opt, char *recoverFileName);

short list_entries(DirEntry *dir);
DirEntry *get_dir_entry(int clusNum);
DirEntry *get_next_dir(DirEntry *dir);
int get_next_clus(int clusNum);
void recover_small_file(char *optarg);
int recover_file(DirEntry *dir, char *recoverFileName);
int match_del_filename(char *delFileName, char *targFileName);

BootEntry *bootEntry;
unsigned char *diskAddr;
int endOfRsvd;
int endOfFAT;
int entriesPerClus;
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
                recover_small_file(optarg);
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
short trav_entries(DirEntry *dir, int opt, char *recoverFileName)
{
    /*
    opt=0: list all entries
    opt=1: recover entries. return -1 if succeed, else return normal count
    */
    short count = 0;
    while (dir->DIR_Name[0] != 0x00 && entriesPerClus > count)
    {
        if (opt == 0)
            count += list_entries(dir);
        else if (opt == 1)
        {
            if (recover_file(dir, recoverFileName) == 1)
                return -1;
        }
        dir++;
    }
    return count;
}

short list_entries(DirEntry *dir)
{
    char fileName[9];
    char fileExt[4];
    if (dir->DIR_Name[0] == 0xE5)
        return 0;
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
    return 1;
}
short trav_dir(DirEntry *dir, int clusNum, int opt, char *recoverFileName)
{
    short entriesCount = 0;
    while (dir)
    {
        if (opt == 0)
            entriesCount += trav_entries(dir, opt, recoverFileName);
        else if (opt == 1)
        {
            if (trav_entries(dir, opt, recoverFileName) == -1)
                return -1;
        }
        if ((clusNum = get_next_clus(clusNum)) == -1)
            break;
        dir = get_dir_entry(clusNum);
    }
    return entriesCount;
}
void list_dir_entries(int clusNum)
{
    DirEntry *rootDir = get_dir_entry(clusNum);
    short entriesCount = trav_dir(rootDir, clusNum, 0, NULL);
    printf("Total number of entries = %d\n", entriesCount);
}
int get_next_clus(int clusNum)
{
    if (FAT[clusNum] >= 0x0ffffff8)
        return -1;
    return FAT[clusNum];
}

void recover_small_file(char *fileName)
{
    int clusNum = (int)bootEntry->BPB_RootClus;
    DirEntry *rootDir = get_dir_entry(clusNum);
    if (trav_dir(rootDir, clusNum, 1, fileName) == -1)
        printf("%s: successfully recovered\n", fileName);
    else
        printf("%s: file not found\n", fileName);
}

int recover_file(DirEntry *dir, char *recoverFileName)
{
    char delFileName[9];
    char delFileExt[4];
    char fullName[14];
    get_name(dir->DIR_Name, delFileName, delFileExt);
    strcpy(fullName, delFileName);
    if (strcmp(delFileExt, "\0") != 0)
    {
        strcat(fullName, ".");
        strcat(fullName, delFileExt);
    }
    if ((fullName[0] == 0xE5) && match_del_filename(fullName, recoverFileName))
    {
        int clusNum = dir->DIR_FstClusHI << 16 | dir->DIR_FstClusLO;
        dir->DIR_Name[0] = recoverFileName[0];
        FAT[clusNum] = 0x0ffffff8;
        return 1;
    }
    return 0;
}
int match_del_filename(char *delFileName, char *targFileName)
{
    short idx = 1;
    while (delFileName[idx] != '\0' && targFileName[idx] != '\0')
    {
        if (delFileName[idx] != targFileName[idx])
            return 0;
        idx++;
    }
    if (delFileName[idx] == '\0' && targFileName[idx] == '\0')
    {
        return 1;
    }
    return 0;
}