#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <openssl/sha.h>
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
void recover_file(char *fileName, char *sha);
short trav_dir_recover(DirEntry *dir, int clusNum, char *recoverFileName, DirEntry *cands[]);
int find_del_file_cand(DirEntry *dir, char *recoverFileName, DirEntry *cands[], short candNum);
int match_del_filename(char *delFileName, char *targFileName);
void recover(DirEntry *dirs, char *fileName);
short recover_with_sha(DirEntry *dirs[], short candNum, char *fileName, char *sha);

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
how to read sha1 in 40 hex digits: https://stackoverflow.com/questions/47605867/how-to-make-sha1-in-c-that-returns-raw-output, https://stackoverflow.com/questions/43696184/wrong-sha1-output-in-hex
*/
int main(int argc, char **argv)
{
    if (argc < 3 || argc > 6)
    {
        print_usage();
        return 0;
    }
    int ch;
    char select = 0;
    char *fileName = NULL;
    char *sha = NULL;
    while ((ch = getopt(argc, argv, "ilr:R:s:")) != -1)
    {
        switch (ch)
        {
        case 'i':
            if (select != 0)
            {
                print_usage();
                exit(1);
            }
            select = 'i';
            break;
        case 'l':
            if (select != 0)
            {
                print_usage();
                exit(1);
            }
            select = 'l';
            break;
        case 'r':
            if (select != 0)
            {
                print_usage();
                exit(1);
            }
            select = 'r';
            fileName = optarg;
            break;
        case 'R':
            if (select != 0)
            {
                print_usage();
                exit(1);
            }
            select = 'R';
            fileName = optarg;
            break;
        case 's':
            if (select == 'R' || select == 'r')
                sha = optarg;
            else
            {
                print_usage();
                exit(1);
            }
            break;
        default:
            print_usage();
            exit(1);
        }
    }
    if (argv[optind] == NULL)
    {
        print_usage();
        exit(1);
    }
    char *disk = argv[optind];
    read_disk(disk);
    if (select == 'i')
        list_fs_info();
    else if (select == 'l')
        list_dir_entries((int)bootEntry->BPB_RootClus);
    else if (select == 'r' || select == 'R')
    {
        if (select == 'R' && sha == NULL)
        {
            print_usage();
            exit(1);
        }
        recover_file(fileName, sha);
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
    short allCount = 0;
    while (dir->DIR_Name[0] != 0x00 && entriesPerClus > allCount)
    {
        char fileName[14];
        allCount++;
        if (dir->DIR_Name[0] == 0xE5)
        {
            dir++;
            continue;
        }
        get_name(dir->DIR_Name, fileName);
        int clusNum = dir->DIR_FstClusHI << 16 | dir->DIR_FstClusLO;
        if (dir->DIR_Attr == 0x10)
            printf("%s/ (starting cluster = %d)\n", fileName, clusNum);
        else
        {
            printf("%s (size = %d", fileName, dir->DIR_FileSize);
            if (dir->DIR_FileSize == 0)
                printf(")\n");
            else
            {
                printf(", starting cluster = %d)\n", clusNum);
            }
        }
        count++;
        dir++;
    }
    return count;
}

short trav_dir(DirEntry *dir, int clusNum)
{
    short entriesCount = 0;
    while (1)
    {
        entriesCount += trav_entries(dir);
        if ((clusNum = get_next_clus(clusNum)) == -1)
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

void recover_file(char *fileName, char *sha)
{
    // printf("recovering file %s with sha %s\n", fileName, sha);
    int clusNum = (int)bootEntry->BPB_RootClus;
    DirEntry *rootDir = get_dir_entry(clusNum);
    DirEntry *cands[100];
    short candNum = trav_dir_recover(rootDir, clusNum, fileName, cands);
    if (sha == NULL)
    {
        if (candNum == 1)
        {
            recover(cands[0], fileName);
            printf("%s: successfully recovered\n", fileName);
        }
        else if (candNum > 1)
        {
            printf("%s: multiple candidates found\n", fileName);
        }
        else
            printf("%s: file not found\n", fileName);
    }
    else
    {
        if (recover_with_sha(cands, candNum, fileName, sha) == 1)
            printf("%s: successfully recovered with SHA-1\n", fileName);
        else
            printf("%s: file not found\n", fileName);
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
        if (((unsigned char)delFileName[0] == 0xE5) && match_del_filename(delFileName, recoverFileName))
        {
            cands[candNum] = dir;
            candNum++;
        }
        count++;
        dir++;
    }
    return candNum;
}

void recover(DirEntry *dir, char *fileName)
{
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

short recover_with_sha(DirEntry *dirs[], short candNum, char *fileName, char *sha)
{
    for (short i = 0; i < candNum; i++)
    {
        int clusNum = dirs[i]->DIR_FstClusHI << 16 | dirs[i]->DIR_FstClusLO;
        unsigned char *data = dataArea + (clusNum - 2) * clusSize;
        unsigned char hash[SHA_DIGEST_LENGTH];
        char buf[SHA_DIGEST_LENGTH * 2];
        size_t size = dirs[i]->DIR_FileSize;
        SHA1(data, size, hash);
        for (int j = 0; j < SHA_DIGEST_LENGTH; j++)
        {
            sprintf((char *)&(buf[j * 2]), "%02x", hash[j]);
        }
        if (strcmp(buf, sha) == 0)
        {
            recover(dirs[i], fileName);
            return 1;
        }
    }
    return 0;
}