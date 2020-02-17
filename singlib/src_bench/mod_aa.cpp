#include <sing.h>
#include <stdio.h>
#include "mod_aa.h"     // myself
//#include "mod_bb.h"     // required stuff referred by private.

namespace mod_aa {

/**** private stuff ****/

// consts
static const int aa_const = 100;
static const int aa_const2 = sizeof(mod_bb::bb);

// types
typedef int mydef;

// prototypes
static void localfun(void);

// **** variables and functions (public and private in the same sequence of the sing file) ******
static mydef myvar = aa_const;  // private

aa mypubvar;                    // public
int mypubvar2 = aa_const;

static void localfun(void)      // private
{
    mod_bb::bb  concrete;

    printf("aa");
    mod_bb::pubfun();
}

void aa::Init()                 // public
{

}
void aa::Shutdown()
{
    mod_bb::pubfun();

}

void pubfun(void)
{
    mod_bb::bb  concrete;
    localfun();
    mod_bb::pubfun();
}

}
