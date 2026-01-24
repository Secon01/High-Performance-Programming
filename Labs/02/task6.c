#include <stdio.h>
#include <math.h>

int main()
{
    int dividend, divisor;
    printf("Enter dividend:");
    scanf("%d", &dividend);
    printf("\nEnter divisor:");
    scanf("%d", &divisor);
    printf("\nQuotient: %d", (int)(dividend/divisor));
    printf("\nRemainder: %d", dividend%divisor);
    return 0;
}