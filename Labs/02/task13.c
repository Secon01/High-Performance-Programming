#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE  *f = fopen("data.txt", "r");
    if(!f) return 1;
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