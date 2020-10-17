#include "sing.h"
#include <float.h>
#include "limits.h"

float xx = FLT_MAX;

// void print_primes_to(int32_t top);
// uint32_t uint32_sqrt(uint32_t x);
// uint16_t SquareRoot(uint32_t x);

void ptrtest(void);
// void CheckLiterals(void);
void using_strings(void);
// void test_types_and_vars();
void test_ptr_speed();
void test_intrinsics(void);
void speed_test(void);
void test_map(void);
void test_std_vectors(int size);
bool str_test();
bool SortTest(int veclen);
bool sio_test();
bool sys_test();
bool thread_test();

int main() {
    //test_types_and_vars();
    //sinth_test();
    //using_strings();
    //test_intrinsics();
    //CheckLiterals();
    ptrtest();
    //print_primes_to(100);
    //test_ptr_speed();
    //speed_test();
    //test_map();
    //test_std_vectors(10);
    // if (str_test()) {
    //     printf("string lib: passed\r\n");
    // } else {
    //     printf("string lib: failed !!\r\n");
    // }
    // if (SortTest(100)) {
    //     printf("\nsort lib: passed\r\n");
    // } else {
    //     printf("\nsort lib: failed !!\r\n");
    // }
    // if (sio_test()) {
    //     printf("\nsio lib: passed\r\n");
    // } else {
    //     printf("\nsio lib: failed !!\r\n");
    // }
    // if (sys_test()) {
    //     printf("\nsys lib: passed\r\n");
    // } else {
    //     printf("\nssys lib: failed !!\r\n");
    // }

    // print limits
    //printf("\nmax float is %g and max double is %g", sing::f32_max, sing::f64_max);    

    // if (thread_test()) {
    //     printf("\nthread lib: passed\r\n");
    // } else {
    //     printf("\nthread lib: failed !!\r\n");
    // }

    return(0);
}