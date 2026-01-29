#include <stdio.h>
#include <stdlib.h>

void print_int_1(int x) {
    printf("Here is the number: %d\n", x);
}
void print_int_2(int x) 
{
    printf("Wow, %d is really an impressive number!\n", x);
}

int main(void)
{
    void (*print_int)(int);
    print_int = print_int_1;
    print_int(1);
    print_int = print_int_2;
    print_int(1);
    return 0;
}