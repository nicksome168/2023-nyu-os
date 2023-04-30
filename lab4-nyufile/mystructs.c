
#pragma pack(push, 1)
typedef struct BootEntry
{
    unsigned char BS_jmpBoot[3];
    unsigned char BS_OEMName[8];
    unsigned short BPB_BytsPerSec;
    unsigned char BPB_SecPerClus;
    unsigned short BPB_RsvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned short BPB_RootEntCnt;
    unsigned short BPB_TotSec16;
    unsigned char BPB_Media;
    unsigned short BPB_FATSz16;
    unsigned short BPB_SecPerTrk;
    unsigned short BPB_NumHeads;
    unsigned int BPB_HiddSec;
    unsigned int BPB_TotSec32;
    unsigned int BPB_FATSz32;
    unsigned short BPB_ExtFlags;
    unsigned short BPB_FSVer;
    unsigned int BPB_RootClus;
    unsigned short BPB_FSInfo;
    unsigned short BPB_BkBootSec;
    unsigned char BPB_Reserved[12];
    unsigned char BS_DrvNum;
    unsigned char BS_Reserved1;
    unsigned char BS_BootSig;
    unsigned int BS_VolID;
    unsigned char BS_VolLab[11];
    unsigned char BS_FilSysType[8];
} BootEntry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct DirEntry
{
    unsigned char DIR_Name[11];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth; /* Created time (tenths of second) */
    unsigned short DIR_CrtTime;     /* Created time (hours, minutes, seconds) */
    unsigned short DIR_CrtDate;     /* Created day */
    unsigned short DIR_LstAccDate;  /* Accessed day */
    unsigned short DIR_FstClusHI;   /* High 2 bytes of the first cluster address */
    unsigned short DIR_WrtTime;     /* Written time (hours, minutes, seconds */
    unsigned short DIR_WrtDate;     /* Written day */
    unsigned short DIR_FstClusLO;   /* Low 2 bytes of the first cluster address */
    unsigned int DIR_FileSize;      /* File size in bytes. (0 for directories) */
} DirEntry;
#pragma pack(pop)