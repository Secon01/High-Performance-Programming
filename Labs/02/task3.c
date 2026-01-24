#include <stdio.h>
#include <math.h>
int even(int a, int b)
{
    if (a%2 == 0 && b%2==0)
    {
        return a+b;
    } else
    {
        return a*b;
    }
}

double largestAbs(double a, double b, double c)
{
    if(fabs(a)>fabs(b) && fabs(a)>fabs(c))
    {
        return a;
    }else if (fabs(b)>fabs(a) && fabs(b)>fabs(c))
    {
        return b;
    }else if (fabs(c)>fabs(a) && fabs(c)>fabs(b))
    {
        return c;
    }else
    {
        return a; /*if ties returne the first one*/
    }
    
}

double scndLargestAbs(double a, double b, double c)
{
    double largest, second;

    // initialize
    if (fabs(a) >= fabs(b)) {
        largest = a;
        second  = b;
    } else {
        largest = b;
        second  = a;
    }

    // compare c
    if (fabs(c) > fabs(largest)) {
        second  = largest;
        largest = c;
    } else if (fabs(c) > fabs(second)) {
        second = c;
    }

    return second;
}

int main(void)
{
    /*
    int a, b;
    printf("Input:");
    scanf("%d %d", &a, &b);
    printf("%d", even(a, b));

    double c, d, e;
    printf("\nInput:");
    scanf("%lf %lf %lf", &c, &d, &e);
    printf("%lf", largestAbs(c, d, e));
*/
    double f, g, h;
    printf("\nInput:");
    scanf("%lf %lf %lf", &f, &g, &h);
    printf("%lf", scndLargestAbs(f, g, h));
    return 0;    
}