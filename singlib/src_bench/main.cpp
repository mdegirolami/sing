#include <sing.h>
//#include <limits.h>

// void print_primes_to(int32_t top);
// uint32_t uint32_sqrt(uint32_t x);
// uint16_t SquareRoot(uint32_t x);

void ptrtest(void);
// void CheckLiterals(void);
void test_vectors(int size);
void using_strings(void);
// void test_types_and_vars();
void test_ptr_speed();
void test_intrinsics(void);
void speed_test(void);
void test_map(void);
void test_std_vectors(int size);

int main() {
    //test_types_and_vars();
    //sinth_test();
    //using_strings();
    //test_intrinsics();
    //test_vectors(5);
    //CheckLiterals();
    //ptrtest();
    //print_primes_to(100);
    //test_ptr_speed();
    //speed_test();
    //test_map();
    test_std_vectors(10);
    return(0);
}