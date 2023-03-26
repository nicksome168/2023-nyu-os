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
// 1 hour: debugging. case 2
// 3/25
// 1 hour: debugging. case 3
//

// research on encode_chunck() interface to be used in pthread
// decouple _encode() from encode_chunck()

// how to use getopt: https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
// how to use pthread: http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html

#define MAX_FILE_SIZE (1024 * 1024 * 1024) // 1GB
#define CHUNK_SIZE (1024 * 4)              // 4kb
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define UNUSED(x) (void)(x)

typedef struct
{
    unsigned char *encs;
    size_t size;
} data;

pthread_mutex_t todoTaskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t doneTaskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t isEnough = PTHREAD_COND_INITIALIZER;
char todoTaskQueue[MAX_FILE_SIZE];
data *doneTaskQueue[MAX_FILE_SIZE / CHUNK_SIZE + 1];
size_t doneTaskQueueSize = 0;
size_t todoTaskQueueTail = 0;
size_t todoTaskQueueHead = 0;
int doneIO = 0;

void writeStd(char c, unsigned char count);
void handle_error(char *err_msg);
void encode(size_t chunkHead, size_t chunkSize);
void *encode_chunk();
void output();
void freeMem();

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
    while ((c = getopt(argc, argv, "j:")) != -1)
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
    int workerId[workerNum];
    for (int i = 0; i < workerNum; i++)
    {
        workerId[i] = i;
        pthread_create(&workerThread[i], NULL, encode_chunk, &workerId[i]);
    }
    /*READ DATA*/
    int fd = 0;
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
        size_t offset = 0;
        size_t fileSize = (size_t)sb.st_size;
        size_t chunkSize = 0;
        // printf("file size: %zu\n", fileSize);
        while (offset < fileSize)
        {
            chunkSize = CHUNK_SIZE * (workerNum * 2);
            chunkSize = MIN(fileSize - offset, chunkSize);
            pthread_mutex_lock(&todoTaskMutex);
            memcpy(todoTaskQueue + todoTaskQueueTail, addr + offset, chunkSize);
            todoTaskQueueTail += chunkSize;
            if (todoTaskQueueHead + CHUNK_SIZE <= todoTaskQueueTail)
                pthread_cond_signal(&isEnough);
            pthread_mutex_unlock(&todoTaskMutex);
            offset += chunkSize;
        }
        // memcpy(todoTaskQueue + todoTaskQueueTail, addr, sb.st_size);
        // pthread_mutex_lock(&todoTaskMutex);
        // todoTaskQueueTail += sb.st_size;
        // if (todoTaskQueueHead + CHUNK_SIZE <= todoTaskQueueTail)
        //     pthread_cond_signal(&isEnough);
        // pthread_mutex_unlock(&todoTaskMutex);
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
    /*MERGE RESULTS*/
    output();
    freeMem();
    return 0;
}
void output()
{
    char lastChar = -1;
    unsigned char lastCount = 0;
    for (size_t i = 0; i < doneTaskQueueSize; i++)
    {
        size_t chunkSize = doneTaskQueue[i]->size - 2;
        if (i == doneTaskQueueSize - 1)
            chunkSize += 2;
        // printf("chunk %zu size: %zu", i, chunkSize);
        if (lastChar != (char)-1)
        {
            if (lastChar == doneTaskQueue[i]->encs[0])
                doneTaskQueue[i]->encs[1] += lastCount;
            else
                writeStd(lastChar, lastCount);
        }
        if (fwrite(doneTaskQueue[i]->encs, sizeof(unsigned char), chunkSize, stdout) != chunkSize)
        {
            handle_error("fwrite failed");
        }
        lastChar = doneTaskQueue[i]->encs[chunkSize];
        lastCount = doneTaskQueue[i]->encs[chunkSize + 1];
    }
}

void writeStd(char c, unsigned char count)
{
    if (fwrite(&c, sizeof(char), 1, stdout) != 1)
    {
        handle_error("fwrite failed");
    }
    if (fwrite(&count, sizeof(unsigned char), 1, stdout) != 1)
    {
        handle_error("fwrite failed");
    }
}

void freeMem()
{
    for (size_t i = 0; i < doneTaskQueueSize; i++)
    {
        // printf("freeing chunk %zu\n", i);
        free(doneTaskQueue[i]->encs);
        free(doneTaskQueue[i]);
    }
}

void *encode_chunk(void *workerId)
{
    int _id = *(int *)workerId;
    UNUSED(_id);
    for (;;)
    {
        size_t head = 0;
        size_t chunkSize = 0;
        pthread_mutex_lock(&todoTaskMutex);
        while (!doneIO && (todoTaskQueueHead + CHUNK_SIZE > todoTaskQueueTail))
            pthread_cond_wait(&isEnough, &todoTaskMutex);
        if (todoTaskQueueHead >= todoTaskQueueTail) // empty
        {
            // printf("no more data\n");
            pthread_mutex_unlock(&todoTaskMutex);
            pthread_cond_broadcast(&isEnough);
            break;
        }
        else
        {
            head = todoTaskQueueHead;
            chunkSize = MIN(CHUNK_SIZE, todoTaskQueueTail - todoTaskQueueHead);
            todoTaskQueueHead += chunkSize;
        }
        // printf("thread %d encode from %zu to %zu\n", _id, head, todoTaskQueueHead);
        pthread_mutex_unlock(&todoTaskMutex);
        encode(head, chunkSize);
    }
    pthread_exit(NULL);
}

void encode(size_t chunkHead, size_t chunkSize)
{
    // char *tmpChars = malloc(sizeof(char) * (chunkSize * 2));
    unsigned char *tmpEnc = malloc(sizeof(unsigned char) * (chunkSize * 4));
    int idx = 0;
    // tmpChars[idx] = todoTaskQueue[chunkHead];
    tmpEnc[idx] = todoTaskQueue[chunkHead];
    tmpEnc[idx + 1] = 1;
    for (size_t i = chunkHead + 1; i < chunkHead + chunkSize; i++)
    {
        if (todoTaskQueue[i] == tmpEnc[idx])
        {
            tmpEnc[idx + 1]++;
        }
        else
        {
            idx += 2;
            tmpEnc[idx] = todoTaskQueue[i];
            tmpEnc[idx + 1] = 1;
        }
        // printf("char: %c; count: %d\n", tmpEnc[idx], tmpEnc[idx + 1]);
    }
    // printf("size of struct: %zu\n", sizeof(data));
    data *doneChunk = malloc(sizeof(data));
    doneChunk->encs = tmpEnc;
    doneChunk->size = idx + 2;
    pthread_mutex_lock(&doneTaskMutex);
    doneTaskQueueSize++;
    // printf("Encoding chunk from %zu to %zu\n", chunkHead, chunkHead + chunkSize);
    // printf("writing chunk %zu\n", chunkHead / CHUNK_SIZE);
    doneTaskQueue[chunkHead / CHUNK_SIZE] = doneChunk;
    pthread_mutex_unlock(&doneTaskMutex);
}

void handle_error(char *err_msg)
{
    // UNUSED(err_msg);
    fprintf(stderr, "%s", err_msg);
    fflush(stdout);
    freeMem();
    exit(0);
}
