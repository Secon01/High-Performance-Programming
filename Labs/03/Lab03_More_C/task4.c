#include <stdio.h>
#include <stdlib.h>

int CmpDouble(const void* p1, const void* p2)
{
    double a = *(const double*)p1;
    double b = *(const double*)p2;
    if (a < b) return 1;
    if (a > b) return -1;    
    return 0;                 /*descending order*/
}
int main(int argc, char const *argv[])
{
    double arrDouble[] = {9.3, -2.3, 1.2, -0.4, 2, 9.2, 1, 2.1, 0, -9.2};
    int arrlen=10;
    qsort (arrDouble, arrlen, sizeof(double), CmpDouble);
    for (int i = 0; i < arrlen; i++)
    {
        printf("\n%.2f", arrDouble[i]);
    }
    
    return 0;
}
