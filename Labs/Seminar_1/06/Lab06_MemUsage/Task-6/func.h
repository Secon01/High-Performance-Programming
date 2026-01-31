//int f(int k);

// The gcc syntax for declaring f as a pure function is as follows:
// int f(int k) __attribute__((const));

#if defined(__GNUC__)
  #define CONSTFUNC __attribute__((const))
#else
  #define CONSTFUNC
#endif

int f(int k) CONSTFUNC;

int get_counter();
