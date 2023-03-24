#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
// 1 hour: read file using mmap
// 1 hour: concat files using memcpy
// 30 min : encode
// 3/22
// 2.5 hour: multithread
// 3/23
// 1.5 hour: debugging
// 1.5 hour: debugging
// 4.20-

// research on encode_chunck() interface to be used in pthread
// decouple _encode() from encode_chunck()

// how to use getopt: https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
// how to use pthread: http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html

#define CHUNK_SIZE (1024 * 4) // 4kb
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
    char *chars;
    unsigned char *count;
    size_t _size;
    size_t _idx;
} data;

pthread_mutex_t todoTaskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t doneTaskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t isEnough = PTHREAD_COND_INITIALIZER;
char *todoTaskQueue = NULL;
data *doneTaskQueue = NULL;
size_t doneTaskQueueTail = 0;
size_t todoTaskQueueTail = 0;
size_t todoTaskQueueHead = 0;
int doneIO = 0;

size_t read_file_size(int argc, char **argv);
void writeStd(size_t chunkIdx, size_t charIdx);
void handle_error(char *err_msg);
void encode(size_t chunkHead, size_t chunkSize);
void *encode_chunk();

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        handle_error("no files specified");
        return 1;
    }
    int c = 0;
    int workerNum = 1;
    /*READ COMMAND LINE INPUT*/
    while ((c = getopt(argc, argv, "j")) != -1)
    {
        switch (c)
        {
        case 'j':
            workerNum = atoi(optarg);
            break;
        default:
            continue;
        }
    }
    /*SPAWN THREAD WORKERS*/
    pthread_t workerThread[workerNum];
    for (int i = 0; i < workerNum; i++)
    {
        pthread_create(&workerThread[i], NULL, encode_chunk, NULL);
    }
    /*READ DATA*/
    int fd = 0;
    struct stat sb;
    char *addr = NULL;
    size_t TOTAL_FILE_SIZE = read_file_size(argc, argv);
    todoTaskQueue = malloc(TOTAL_FILE_SIZE * sizeof(char));
    doneTaskQueue = malloc(TOTAL_FILE_SIZE * sizeof(data));
    for (int i = optind; i < argc; i++)
    {
        if ((fd = open(argv[i], O_RDONLY)) == -1)
            handle_error("open failed");
        if (fstat(fd, &sb) == -1)
            handle_error("fstat failed");
        if ((addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
            handle_error("mmap failed");
        memcpy(todoTaskQueue + todoTaskQueueTail, addr, sb.st_size);
        pthread_mutex_lock(&todoTaskMutex);
        todoTaskQueueTail += sb.st_size;
        if (todoTaskQueueHead + CHUNK_SIZE <= todoTaskQueueTail)
            pthread_cond_signal(&isEnough);
        // printf("File size %zu\n", sb.st_size);
        pthread_mutex_unlock(&todoTaskMutex);
        if (munmap(addr, sb.st_size) == -1)
            handle_error("munmap failed");
        if (close(fd) == -1)
            handle_error("close failed");
    }
    pthread_mutex_lock(&todoTaskMutex);
    doneIO = 1;
    pthread_cond_signal(&isEnough);
    pthread_mutex_unlock(&todoTaskMutex);
    /*WAIT FOR WORKERS TO FINISH*/
    for (int i = 0; i < workerNum; i++)
    {
        pthread_join(workerThread[i], NULL);
    }
    // // printf("done\n");
    /*MERGE RESULTS*/
    size_t lastItemInPrevChunk = 0;
    for (size_t i = 0; i < doneTaskQueueTail; i++)
    {
        // printf("Chunk %zu with %zu chars\n", i, doneTaskQueue[i]->_size);
        for (size_t j = 0; j < doneTaskQueue[i]._size - 1; j++)
        {
            if (j == 0 && i > 0) // stich result together
            {
                if (doneTaskQueue[i].chars[j] == doneTaskQueue[i - 1].chars[lastItemInPrevChunk])
                    doneTaskQueue[i].count[j] += doneTaskQueue[i - 1].count[lastItemInPrevChunk];
                else
                {
                    writeStd(i - 1, lastItemInPrevChunk);
                }
            }
            writeStd(i, j);
        }
        lastItemInPrevChunk = doneTaskQueue[i]._size - 1;
    }
    if (doneTaskQueueTail > 0)
        writeStd(doneTaskQueueTail - 1, lastItemInPrevChunk);
    for (size_t i = 0; i < doneTaskQueueTail; i++)
    {
        free(doneTaskQueue[i].chars);
        free(doneTaskQueue[i].count);
    }
    free(todoTaskQueue);
    free(doneTaskQueue);
    return 0;
}

size_t read_file_size(int argc, char **argv)
{
    int fd = 0;
    struct stat sb;
    size_t total_file_size = 0;
    for (int i = optind; i < argc; i++)
    {
        if ((fd = open(argv[i], O_RDONLY)) == -1)
            handle_error("open failed");
        if (fstat(fd, &sb) == -1)
            handle_error("fstat failed");
        total_file_size += sb.st_size;
        if (close(fd) == -1)
            handle_error("close failed");
    }
    return total_file_size;
}

void writeStd(size_t chunkIdx, size_t charIdx)
{
    if (fwrite(&doneTaskQueue[chunkIdx].chars[charIdx], sizeof(char), 1, stdout) != 1)
    {
        handle_error("fwrite failed");
    }
    if (fwrite(&doneTaskQueue[chunkIdx].count[charIdx], sizeof(char), 1, stdout) != 1)
    {
        handle_error("fwrite failed");
    }
}

void *encode_chunk()
{
    for (;;)
    {
        size_t chunkSize = 0;
        pthread_mutex_lock(&todoTaskMutex);
        while (!doneIO && (todoTaskQueueHead + CHUNK_SIZE > todoTaskQueueTail))
            pthread_cond_wait(&isEnough, &todoTaskMutex);
        // // printf("thread done waiting\n");
        printf("head: %zu, tail: %zu\n", todoTaskQueueHead, todoTaskQueueTail);
        if (todoTaskQueueHead >= todoTaskQueueTail) // empty
        {
            // printf("no more data\n");
            pthread_mutex_unlock(&todoTaskMutex);
            break;
        }
        else
        {
            chunkSize = MIN(CHUNK_SIZE, todoTaskQueueTail - todoTaskQueueHead);
            todoTaskQueueHead += chunkSize;
        }
        pthread_mutex_unlock(&todoTaskMutex);
        encode(todoTaskQueueHead - chunkSize, chunkSize);
    }
    pthread_exit(NULL);
}

void encode(size_t chunkHead, size_t chunkSize)
{
    char *tmpChars = malloc(sizeof(char) * (chunkSize / 2 + 1));
    unsigned char *tmpCount = malloc(sizeof(unsigned char) * (chunkSize / 2 + 1));
    int idx = 0;
    tmpChars[idx] = todoTaskQueue[chunkHead];
    tmpCount[idx] = 1;
    printf("Encoding chunk from %zu to %zu\n", chunkHead, chunkHead + chunkSize);
    for (size_t i = chunkHead + 1; i < chunkHead + chunkSize; i++)
    {
        // printf("prev char: %c; cur char: %c\n", tmpChars[idx], todoTaskQueue[i]);
        if (todoTaskQueue[i] == tmpChars[idx])
        {
            tmpCount[idx]++;
        }
        else
        {
            idx++;
            tmpChars[idx] = todoTaskQueue[i];
            tmpCount[idx] = 1;
        }
    }
    printf("size of struct: %zu\n", sizeof(data));
    data *doneChunk = malloc(sizeof(data));
    doneChunk->chars = tmpChars;
    doneChunk->count = tmpCount;
    doneChunk->_size = idx + 1;
    doneChunk->_idx = chunkHead;
    pthread_mutex_lock(&doneTaskMutex);
    doneTaskQueue[doneTaskQueueTail++] = *doneChunk;
    pthread_mutex_unlock(&doneTaskMutex);
}

void handle_error(char *err_msg)
{
    fprintf(stderr, "%s", err_msg);
    fflush(stdout);
    exit(1);
}
