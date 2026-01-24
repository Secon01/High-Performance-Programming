#include <stdio.h>
#include <math.h>

int main(void)
{
    double x;
    printf("Input:");
    scanf("%lf", &x);
    int root = (int)sqrt(x);
    if (root * root == x)
    {
        printf("Perfect square!");
    } else
    {
        printf("Not perfect square!");
    }
    return 0; 
}