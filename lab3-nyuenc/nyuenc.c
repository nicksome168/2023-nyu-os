#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// #include <pthread.h>
// 1 hour: read file using mmap
// 1 hour: concat files using memcpy
// 30 min : encode
// research on encode_chunck() interface to be used in pthread
// decouple _encode() from encode_chunck()

// how to use pthread: http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html

#define MAX_FILE_SIZE (1024 * 1024) // 1GB

void handle_error(char *err_msg);
int encode(char *data, size_t size);

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;
    int c = 0;
    int worker = 1;
    while ((c = getopt(argc, argv, "j")) != -1)
    {
        switch (c)
        {
        case 'j':
            worker = atoi(optarg);
            break;
        default:
            continue;
        }
    }
    /*SPAWN THREAD WORKERS*/
    for (int i = 0; i < worker; i++)
    {
        // pthread_create();
    }
    /*READ DATA*/
    int fd = 0;
    char *data = malloc(MAX_FILE_SIZE);
    size_t offset = 0;
    struct stat sb;
    char *addr = NULL;
    for (int i = optind; i < argc; i++)
    {
        if ((fd = open(argv[i], O_RDONLY)) == -1)
            handle_error("open failed");
        if (fstat(fd, &sb) == -1)
            handle_error("fstat failed");
        if ((addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
            handle_error("mmap failed");
        memcpy(data + offset, addr, sb.st_size);
        offset += sb.st_size;
        if (munmap(addr, sb.st_size) == -1)
            handle_error("munmap failed");
        if (close(fd) == -1)
            handle_error("close failed");
    }

    /*PRODUCER SUBMIT TASKS*/
    encode(data, offset);

    /*CONSUMER ENCODE*/

    /*MERGE RESULTS*/
    // if (fwrite(data, sizeof(char), offset, stdout) != offset)
    // {
    //     handle_error("fwrite failed");
    // }
    return 0;
}

int encode(char *data, size_t size)
{
    if (size == 0)
    {
        return -1;
    }
    // store the character in ASCII and the count as a 1-byte unsigned integer in binary format
    // the count as a 1-byte unsigned integer in binary format
    unsigned char count[MAX_FILE_SIZE];
    char chars[MAX_FILE_SIZE];
    char prev = data[0];
    int char_idx = 0;
    count[char_idx] = 1;
    chars[char_idx] = prev;
    for (size_t i = 1; i < size; i++)
    {
        if (data[i] == prev)
        {
            count[char_idx]++;
        }
        else
        {
            prev = data[i];
            char_idx++;
            chars[char_idx] = prev;
            count[char_idx] = 1;
        }
    }
    char_idx++;
    // convert integers in counts from decimal to binary format
    for (int i = 0; i < char_idx; i++)
    {
        count[i] = (unsigned char)count[i];
    }
    // write char and count by index to stdout
    for (int i = 0; i < char_idx; i++)
    {
        if (fwrite(&chars[i], sizeof(char), 1, stdout) != 1)
        {
            handle_error("fwrite failed");
        }
        if (fwrite(&count[i], sizeof(char), 1, stdout) != 1)
        {
            handle_error("fwrite failed");
        }
    }
    return char_idx;
}
void handle_error(char *err_msg)
{
    fprintf(stderr, err_msg);
    fflush(stdout);
    exit(1);
}