#include <stdio.h>

int main(void)
{
    int n, original, reverse = 0;

    printf("Input: ");
    scanf("%d", &n);

    original = n;

    while (n > 0)
    {
        reverse = reverse * 10 + n % 10;
        n = n / 10;
    }

    if (original == reverse)
        printf("it is a palindrome");
    else
        printf("it is not a palindrome");

    return 0;
}