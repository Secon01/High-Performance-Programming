#include <stdio.h>
#include <stdlib.h>

typedef struct product
{
    char name[50];
    double price;
}
product_t;

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
    product_t arr_of_prod[100];
    for (int i = 0; i < n; i++)
    {

        fscanf(f, "%s %lf", arr_of_prod[i].name, &arr_of_prod[i].price);
        printf("\n%s %.2f", arr_of_prod[i].name, arr_of_prod[i].price);
    }
    fclose(f);
    return 0;
}