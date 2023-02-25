#include <stdio.h>
#include <string.h>
int main()
{
    char str1[4] = "1234";
    char str2[4];
    strncpy(str2, str1, sizeof(str2));
    printf("str2 = %s\n", str2);
}