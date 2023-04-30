void get_name(unsigned char *dirName, char *fileName, char *fileExt);
void add_null_term(char *chars, int size);

void handle_error(char *err_msg)
{
    printf("%s\n", err_msg);
    exit(1);
}

void print_usage()
{
    printf("Usage: ./nyufile disk <options>\n\
  -i                     Print the file system information.\n\
  -l                     List the root directory.\n\
  -r filename [-s sha1]  Recover a contiguous file.\n\
  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
    exit(1);
}

void get_name(unsigned char *dirName, char *fileName, char *fileExt)
{
    memcpy(fileName, dirName, 8);
    memcpy(fileExt, dirName + 8, 3);
    add_null_term(fileName, 8);
    add_null_term(fileExt, 3);
}

void add_null_term(char *chars, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (chars[i] == ' ')
        {
            chars[i] = '\0';
            return;
        }
    }
    chars[size] = '\0';
}