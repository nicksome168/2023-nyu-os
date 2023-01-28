#include <stdio.h>
#include <stdlib.h> 

int *add(int a, int b)
{
    int c = a + b;
    int *d = (int*)malloc(sizeof(int*));
    *d = c;
    return d;
}
int main()
{
    int *result = add(1, 2);
    printf("result = %d\n", *result);
    printf("result = %d\n", *result);
}