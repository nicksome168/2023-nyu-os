#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage();
int main(int argc, char **argv)
{

    /*
    Usage: ./nyufile disk <options>
   -i                     Print the file system information.
   -l                     List the root directory.
   -r filename [-s sha1]  Recover a contiguous file.
   -R filename -s sha1    Recover a possibly non-contiguous file.
    */
    if (argc < 3 || argc > 6)
    {
        print_usage();
        return 0;
    }
    // char *disk = argv[1];
    // printf("disk: %s\n", disk);
    int ch;
    int select = 0;
    int selectR = 0;
    int selectS = 0;
    while ((ch = getopt(argc, argv, "ilr:R:s:")) != -1)
    {
        switch (ch)
        {
        case 'i':
            if (select == 0)
            {
                // printf("-i\n");
                select = 1;
            }
            break;
        case 'l':
            if (select == 0)
            {
                // printf("-l\n");
                select = 1;
            }
            break;
        case 'r':
            if (select == 0)
            {
                // printf("-r: %s\n", optarg);
                select = 1;
                selectR = 1;
            }
            break;
        case 'R':
            if (select == 0)
            {
                // printf("-R: %s\n", optarg);
                select = 1;
                selectR = 1;
            }
            break;
        case 's':
            selectS = 1;
            // printf("sha1: %s\n", optarg);
            break;
        default:
            print_usage();
        }
    }
    if (selectR * selectS != 1)
        print_usage();
    return 0;
}

void print_usage()
{
    printf("Usage: ./nyufile disk <options>\n\
  -i                     Print the file system information.\n\
  -l                     List the root directory.\n\
  -r filename [-s sha1]  Recover a contiguous file.\n\
  -R filename -s sha1    Recover a possibly non-contiguous file.");
    exit(1);
}