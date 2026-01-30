#include <stdio.h>
#include <stdint.h>

static inline int add1(int x) {   // compiler can inline this
    return x + 1;
}

int main(void) {
    volatile int s = 0;
    const int N = 200000000;

    for (int i = 0; i < N; i++) {
        s = add1(s);
    }

    printf("%d\n", s);
    return 0;
}