#include <stdio.h>
#include <stdint.h>

__attribute__((noinline)) int add1(int x) {   // force a real call
    return x + 1;
}

int main(void) {
    volatile int s = 0;  // volatile prevents optimizing everything away
    const int N = 200000000;  // big loop

    for (int i = 0; i < N; i++) {
        s = add1(s);
    }

    printf("%d\n", s);
    return 0;
}
