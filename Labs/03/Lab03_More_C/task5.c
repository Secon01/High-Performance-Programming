#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CmpString(const void* p1, const void* p2)
{
    const char *s1 = *(const char**)p1;
    const char *s2 = *(const char**)p2;
    return strcmp(s1, s2);
}

int main(int argc, char const *argv[])
{
    char *arrStr[] = {"daa", "cbab", "bbbb", "bababa", "ccccc", "aaaa"};
    int arrStrLen = sizeof(arrStr) / sizeof(char *);
    qsort(arrStr, arrStrLen, sizeof(char *), CmpString);
    for (int i = 0; i < arrStrLen; i++)
    {
        printf("\n%s", arrStr[i]);
    }
    return 0;
}