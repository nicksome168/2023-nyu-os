#include <pthread.h>
#include <stdio.h>
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *run(void *arg)
{
    pthread_mutex_lock(&mutex);
    int index = *((int *)arg);
    pthread_mutex_unlock(&mutex);
    printf("My index is %d\n", index);
    pthread_exit(NULL);
}
int main()
{
    pthread_t tid[5];
    int args[5];
    for (int i = 0; i < 5; ++i)
    {
        args[i] = i;
        pthread_create(&tid[i], NULL, run, &args[i]);
    }
    for (int i = 0; i < 5; ++i)
    {
        pthread_join(tid[i], NULL);
    }
}
