#include "main.h"
#include "synth_test_oop.h"
#include "synth_test.h"
#include "types_and_vars.h"
#include "siege.h"

int32_t main()
{
    sinth_test_oop::test_oop();
    synth_test();
    test_types_and_vars();
    print_primes_to(100);
    return (1);
}
