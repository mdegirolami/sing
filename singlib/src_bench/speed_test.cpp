#include "sing.h"
#include "stdlib.h"
#include <time.h>
#include <unordered_map>
#include <string>

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
void map_speed(void);
void string_speed(void);

void speed_test(void)
{
    //printf("Hallo world");
    // vect_of_int();
    // vect_of_classes();
    map_speed();
    // string_speed();
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

void map_speed(void)
{
    clock_t start;
    static const int num_keys = 300000;
    int32_t keys[num_keys];

    // build one million random strings (100 chars each)
    for (int ii = 0; ii < num_keys; ++ii) {
        keys[ii] = rand() * rand();
    }

    std::unordered_map<int32_t, int> stdmap;
    sing::map<int32_t, int> singmap;

    // insertion (pointer)
    start = clock();
    for (int ii = 0; ii < num_keys; ++ii) {
        stdmap.insert(std::make_pair(keys[ii], ii));
    }
    printf("\n\nstd::insert = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // extraction(pointer)
    start = clock();
    int sum = 0;
    for (int ii = 0; ii < num_keys; ++ii) {
        sum += stdmap[keys[ii]];
    }
    printf("\n\nstd::access = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // insertion (chars)
    start = clock();
    for (int ii = 0; ii < num_keys; ++ii) {
        singmap.insert(keys[ii], ii);
    }
    printf("\n\nsing::insert = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // extraction (chars)
    start = clock();
    sum = 0;
    for (int ii = 0; ii < num_keys; ++ii) {
        sum += singmap.get(keys[ii]);
    }
    printf("\n\nsing::access = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    // test + extraction (chars)
    start = clock();
    sum = 0;
    for (int ii = 0; ii < num_keys; ++ii) {
        if (singmap.has(keys[ii])) {
            sum += singmap.get(keys[ii]);
        }
    }
    printf("\n\nsing::test + access = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
}

static const int numstr = 300000;
std::string stdstrings[numstr];
sing::string singstrings[numstr];

//
// FIndings:
// std::string is a 32 bytes structure. It saves: pointer, size, 16 bytes string (or allocated: union).
// sing::string is a 16 bytes structure. It saves: pointer, allocated.
//
// init with const char: sing::string just saves the pointer is faster if strlen > 15, else is slower
// append: sing::string is ultraslow because not having the size of the string (easy to add) must do strlen().
// small (<15) nonconst strings: std:: is much faster.
// parameter passing a literal string: sing:: is some faster (half the data). much faster is string > 15 chars 
//    -> can be overcome if all input strings are const char *  
//
void string_speed(void)
{
    clock_t start;

    start = clock();
    for (int ii = 0; ii < numstr; ii += 3) {
        stdstrings[ii] = "the first longggggggggggggggggggg";
        stdstrings[ii+1] = "the second longggggggggggggggggggg";
        stdstrings[ii+2] = "the third longggggggggggggggggggg";
    }
    printf("\n\nstd::init = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    start = clock();
    for (int ii = 0; ii < numstr; ii += 3) {
        singstrings[ii] = "the first longggggggggggggggggggg";
        singstrings[ii+1] = "the second longggggggggggggggggggg";
        singstrings[ii+2] = "the third longggggggggggggggggggg";
    }
    printf("\n\nsing::init = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
/*
    start = clock();
    std::string acc;
    for (int ii = 0; ii < numstr; ii += 3) {
        acc += stdstrings[ii];
        acc += stdstrings[ii+1];
        acc += stdstrings[ii+2];
    }
    printf("\n\nstd::concatenate = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);

    start = clock();
    sing::string singacc;
    for (int ii = 0; ii < numstr; ii += 3) {
        singacc += singstrings[ii];
        singacc += singstrings[ii+1];
        singacc += singstrings[ii+2];
    }
    printf("\n\nsing::concatenate = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
    */
}