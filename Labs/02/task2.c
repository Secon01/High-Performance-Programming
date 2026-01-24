#include <stdio.h>

int main(void)
{
    int a, b;
    printf("Input:");
    scanf("%d %d", &a, &b);
    int i, j;
    for (i = 0; i < a; i++)        
    {
        for (j = 0; j < b; j++)
        {
            if(i == 0 || i == a - 1 || j==0 || j == b - 1)
            {
                printf("*");
            } else
            {
                printf(".");
            }
        }
        printf("\n");        
    }
    
    return 0;
}