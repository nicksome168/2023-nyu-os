#include <stdio.h>
#include <string.h>
int main()
{
    int arr1[4] = {1, 2, 3, 4};
    int arr2[4];
    int arr3[4];
    strncpy((char *)arr2, (char *)arr1, 16);
    memcpy(arr3, arr1, 4 * sizeof(int));

    printf("arr2 = ");
    for (int i = 0; i < 4; ++i)
    {
        printf("%d", arr2[i]);
    }
    printf("\n");
    for (int i = 0; i < 4; ++i)
    {
        printf("%d", arr3[i]);
    }
}