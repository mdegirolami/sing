#include "sing.h"
//#include <memory>
#include <time.h>

class toconstruct {
public:    
    toconstruct() {
        value = (int)sqrt(1000);
    }
    int getvalue(void) { return(value); }
private:
    int value;
};

void vect_of_int(void);
void vect_of_classes(void);

void speed_test(void)
{
    vect_of_int();
    vect_of_classes();
}

void vect_of_int(void)
{
    std::vector<int> totest;
    sing::dvect<int> totest_sing;
    clock_t start;

    // push_back
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest.push_back(ii);
    }
    printf("\n\nstd::push_back = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest_sing.push_back(ii);
    }
    printf("\nsing::push_back = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // indirection
    start = clock();
    int ii;
    for (ii = 0; ii < 10000000; ++ii) {
        if (totest.at(ii) == 9999999) break;
    }
    printf("\n\nstd::reference = %d, %d", (clock() - start) * 1000 / CLOCKS_PER_SEC, ii);
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        if (totest_sing[ii] == 9999999) break;
    }
    printf("\nsing::reference = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // push_back
    totest.clear();
    totest_sing.clear();
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest.push_back(ii);
    }
    printf("\n\nstd::push_back reserved = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest_sing.push_back(ii);
    }
    printf("\nsing::push_back reserved = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
}

void vect_of_classes(void)
{
    std::vector<toconstruct> totest;
    sing::dvect<toconstruct> totest_sing;
    clock_t start;
    toconstruct toinsert;

    // push_back
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest.push_back(toinsert);
    }
    printf("\n\nstd::push_back = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest_sing.push_back(toinsert);
    }
    printf("\nsing::push_back = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // indirection
    start = clock();
    int ii;
    for (ii = 0; ii < 10000000; ++ii) {
        if (totest.at(ii).getvalue() == -1) break;
    }
    printf("\n\nstd::reference = %d, %d", (clock() - start) * 1000 / CLOCKS_PER_SEC, ii);
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        if (totest_sing[ii].getvalue() == -1) break;
    }
    printf("\nsing::reference = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // push_back
    totest.clear();
    totest_sing.clear();
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest.push_back(toinsert);
    }
    printf("\n\nstd::push_back reserved = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
    start = clock();
    for (int ii = 0; ii < 10000000; ++ii) {
        totest_sing.push_back(toinsert);
    }
    printf("\nsing::push_back reserved = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
}
