#include <stdio.h>
#include <string.h>
size_t f(const char c[])
{
  return strlen(c);
}
int main()
{
  const char a[20] = "Hello, 2250!";
  const char *b = "Hello, 2250!";
  printf("size of a is %lu\n", sizeof(a));
  printf("size of b is %lu\n", sizeof(b));
  printf("f(a) returns %lu\n", f(a));
  printf("f(b) returns %lu\n", f(b));
}