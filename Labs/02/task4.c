#include <stdio.h>

double calculator(double a, double b, char op)
{
    switch (op)
    {
    case '+':
        return a + b;
    case '-':
        return a - b;
    case '*':
        return a * b;
    case '/':
        return a / b;
    default:
        return 0;
    }
}


int main(void)
{
    double a,b;
    char op;
    printf("Input:\n");
    scanf("%lf" "%c" "%lf", &a, &op, &b);
    printf("%lf", calculator(a, b, op));
    return 0;
}