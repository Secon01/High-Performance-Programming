#include <stdio.h>

double price = 10.5;
int number = 6;
char alpha = 'a';
int main(void)
{
    printf("%p %p %p", (void*)&price, (void*)&number, (void*)&alpha);
    printf("\n%zu %zu %zu", sizeof(price), sizeof(number), sizeof(alpha));        
    return 0;
}