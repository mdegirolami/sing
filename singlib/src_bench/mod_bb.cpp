#include <sing.h>
#include <stdio.h>
#include "mod_bb.h"     // myself
//#include "mod_aa.h"     // required stuff referred by private.

namespace mod_bb {

/**** private stuff ****/

// consts
static const int bb_const = 100;
static const int bb_const2 = sizeof(mod_aa::aa);

// types
typedef int mydef;

// prototypes
static void localfun(void);

// **** variables and functions (public and private in the same sequence of the sing file) ******
static mydef myvar = bb_const;  // private

bb mypubvar;                    // public
int mypubvar2 = bb_const;

static void localfun(void)      // private
{
    mod_aa::aa  concrete;
    printf("bb");
    mod_aa::pubfun();
}

void bb::Init()                 // public
{
    mod_aa::pubfun();

}
void bb::Shutdown()
{

}

void pubfun(void)
{
    mod_aa::aa  concrete;
    localfun();
    mod_aa::pubfun();
}

}
