#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    if (argc != 2) 
    {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];
    FILE  *f = fopen(filename, "r");
    if(!f) 
    {
        perror("fopen");
        return 1;
    }
    int n;
    fscanf(f, "%d", &n);
    char product[50];
    double price;
    for (int i = 0; i < n; i++)
    {
        fscanf(f, "%s %lf", product, &price);
        printf("\n%s %.2f", product, price);
    }
    fclose(f);
    return 0;
}