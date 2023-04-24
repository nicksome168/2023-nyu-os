int main()
{
    char a = 'a';
    char *b = &a;
    char *c = "c";
    char d[] = "d";
    char *e = d;
    a = 'A';    // (a)
    b[0] = 'B'; // (b)
    c[0] = 'C'; // (c)
    // d[0] = 'D'; // (d)
    // e[0] = 'E'; // (e)
}
