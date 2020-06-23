#include "siege.h"

static uint32_t uint32_sqrt(const uint32_t x);

void print_primes_to(const int32_t top)
{
    std::vector<int32_t> primes;

    // note: for all the numbers in the range excluing even numbers
    for(int32_t totry = 3, totry__top = top; totry < totry__top; totry += 2) {
        const int32_t max_val = (int32_t)uint32_sqrt((uint32_t)totry);          // max divisor who need to check
        bool isprime = true;

        for(auto &value : primes) {
            if (value > max_val) {
                break;
            }
            if (totry % value == 0) {
                isprime = false;
                break;
            }
        }
        if (isprime) {
            primes.push_back(totry);
        }
    }
}

// bisection
static uint32_t uint32_sqrt(const uint32_t x)
{
    uint32_t res = (uint32_t)0;
    uint32_t add = (uint32_t)0x8000;

    for(int32_t ii = 0; ii < 16; ++ii) {
        const uint32_t temp = res | add;

        if (x >= temp * temp) {
            res = temp;
        }
        add >>= (uint32_t)1;
    }
    return (res);
}
