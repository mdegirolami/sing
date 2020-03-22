#include "siege.h"

static uint32_t uint32_sqrt(const uint32_t x);
static uint16_t SquareRoot(const uint32_t x);

void print_primes_to(const int32_t top)
{
    sing::dpvect<int32_t> primes;
    int32_t primes_count = 0;

    // note: for all the numbers in the range excluing even numbers
    for(int32_t totry = 3, totry__top = top; totry < totry__top; totry += 2) {
        const int32_t max = (int32_t)uint32_sqrt((uint32_t)totry);              // max divisor who need to check
        bool isprime = true;

        for(int32_t *value = primes.begin(); value < primes.end(); ++value) {
            if (*value > max) {
                break;
            }
            if (totry % *value == 0) {
                isprime = false;
                break;
            }
        }
        if (isprime) {
            primes[primes_count] = totry;
            ++primes_count;
        }
    }
}
            // print(totry);

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

static uint16_t SquareRoot(const uint32_t x)
{
    uint32_t op = x;
    uint32_t res = (uint32_t)0;
    uint32_t one = (uint32_t)(1 << 30); // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type

    // "one" starts at the highest power of four <= than the argument.
    while (one > op) {
        one >>= (uint32_t)2;
    }

    while (!sing::iseq(one, 0)) {
        if (op >= res + one) {
            op = op - (res + one);
            res = res + 2U * one;
        }
        res >>= (uint32_t)1;
        one >>= (uint32_t)2;
    }
    return ((uint16_t)res);
}

// 2 bits at a time
