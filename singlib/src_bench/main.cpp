#include "sing.h"
#include <float.h>
#include "values.h"

float xx = FLT_MAX;

void AllStdPtrOperations(void);
void test_intrinsics(void);
void speed_test(void);
void test_map(void);
void test_std_vectors(int size);
bool str_test();
bool SortTest(int veclen);
bool sio_test();
bool sys_test();
bool thread_test();
bool net_test();
bool jsonTest();
void test_refguard(void);

int main() {
    // test_intrinsics();
    // AllStdPtrOperations();
    // speed_test();
    // test_map();
    // test_std_vectors(10);
    
    //test_refguard();
    //return(0);

    if (str_test()) {
        printf("string lib: passed\r\n");
    } else {
        printf("string lib: failed !!\r\n");
    }
    if (SortTest(20)) {
        printf("\nsort lib: passed\r\n");
    } else {
        printf("\nsort lib: failed !!\r\n");
    }
    if (sio_test()) {
        printf("\nsio lib: passed\r\n");
    } else {
        printf("\nsio lib: failed !!\r\n");
    }
    if (sys_test()) {
        printf("\nsys lib: passed\r\n");
    } else {
        printf("\nssys lib: failed !!\r\n");
    }

    // print limits
    printf("\nmax float is %g and max double is %g", sing::f32_max, sing::f64_max);    
 
    if (thread_test()) {
        printf("\nthread lib: passed\r\n");
    } else {
        printf("\nthread lib: failed !!\r\n");
    }
 
    if (net_test()) {
        printf("\nnet lib: passed\r\n");
    } else {
        printf("\nnet lib: failed !!\r\n");
    }


    if (jsonTest()) {
        printf("\njson lib: passed\r\n");
    } else {
        printf("\njson lib: failed !!\r\n");
    }

    printf("\npress any key to exit\r\n");
    getchar();

    return(0);
}