#ifndef MOD_AA_H__GUARD
#define MOD_AA_H__GUARD

#include "mod_bb.h"     // required stuff referred by public stuff

namespace mod_aa {

static const int pubconst = 101;

class aa {
    mod_bb::bb  *ptr;
public:
    void Init();
    void Shutdown();
};

void pubfun(void);

extern aa mypubvar;
extern int mypubvar2;

}; // namespace
#else
namespace mod_aa {
class aa;
}
#endif
