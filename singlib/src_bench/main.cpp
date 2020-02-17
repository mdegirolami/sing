#include <sing.h>
//#include <limits.h>
#include "sinth_test.h"

void main();
void print_primes_to(int32_t top);
uint32_t uint32_sqrt(uint32_t x);
uint16_t SquareRoot(uint32_t x);

void ptrtest(void);
void CheckLiterals(void);
void test_vectors(int size);
void using_strings(void);
void test_types_and_vars();

void main() {
    test_types_and_vars();
    sinth_test();
    using_strings();
    test_vectors(5);
    CheckLiterals();
    ptrtest();
    print_primes_to(100);
}