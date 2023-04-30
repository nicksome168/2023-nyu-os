void get_name(unsigned char *dirName, char *fileName);
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

void get_name(unsigned char *dirName, char *fileName)
{
    int idx = 0;
    while (dirName[idx] != ' ' && idx < 8)
    {
        fileName[idx] = dirName[idx];
        idx++;
    }
    int dirIdx = idx;
    while (dirName[dirIdx] == ' ' && dirIdx < 11)
        dirIdx++;
    if (dirIdx < 11)
    {
        fileName[idx] = '.';
        idx++;
    }
    while (dirIdx < 11 && dirName[dirIdx] != ' ')
    {
        fileName[idx] = dirName[dirIdx];
        idx++;
        dirIdx++;
    }
    fileName[idx] = '\0';
}

int match_del_filename(char *delFileName, char *targFileName)
{
    short idx = 1;
    while (delFileName[idx] != '\0' && targFileName[idx] != '\0')
    {
        if (delFileName[idx] != targFileName[idx])
            return 0;
        idx++;
    }
    if (delFileName[idx] == '\0' && targFileName[idx] == '\0')
    {
        return 1;
    }
    return 0;
}