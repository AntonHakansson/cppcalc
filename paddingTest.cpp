#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>

struct test {
  union {
    double a;
    char* d;
  };
  char c;
  char b;
};


int main() {
  printf("c: s: %lld o: %lld\n", sizeof(test::c), offsetof(test, c));
  printf("a: s: %lld o: %lld\n", sizeof(test::a), offsetof(test, a));
  printf("d: s: %lld o: %lld\n", sizeof(test::d), offsetof(test, d));
  printf("b: s: %lld o: %lld\n", sizeof(test::b), offsetof(test, b));

  printf("%lld\n", sizeof(test));
  return 0;
}
