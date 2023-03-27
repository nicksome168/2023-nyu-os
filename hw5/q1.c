#include <pthread.h>
#include <stdio.h>
void *run(void *arg)
{
    printf("child: %d\n", *((int *)arg));
    *((int *)arg) = 2;
    printf("child: %d\n", *((int *)arg));
    pthread_exit(NULL);
}
int main()
{
    pthread_t tid;
    int arg = 1;
    printf("main: %d\n", arg);
    pthread_create(&tid, NULL, run, &arg);
    pthread_join(tid, NULL);
    printf("main: %d\n", arg);
}
