#include "sing.h"
#include "stdlib.h"
#include <time.h>
#include <unordered_map>
#include <string>

void map_speed(void);
void string_speed(void);
void format_speed(void);

void speed_test(void)
{
    map_speed();
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

//
// Comparison of std::string and sing:string findings:
// std::string is a 32 bytes structure. It saves: pointer, size, 16 bytes string (or allocated: union).
// sing::string is a 16 bytes structure. It saves: pointer, allocated.
//
// init with const char: sing::string just saves the pointer is faster if strlen > 15, else is slower
// append: sing::string is ultraslow because not having the size of the string (easy to add) must do strlen().
// small (<15) nonconst strings: std:: is much faster.
// parameter passing a literal string: sing:: is some faster (half the data). much faster is string > 15 chars 
//    -> can be overcome if all input strings are const char *  
//