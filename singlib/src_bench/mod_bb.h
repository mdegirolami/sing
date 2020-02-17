#ifndef MOD_BB_H__GUARD
#define MOD_BB_H__GUARD

#include "mod_aa.h"     // required stuff referred by public stuff

namespace mod_bb {

static const int pubconst = 101;

class bb {
    mod_aa::aa  *ptr;
public:
    void Init();
    void Shutdown();
};

void pubfun(void);

extern bb mypubvar;
extern int mypubvar2;

}; // namespace
#else
namespace mod_bb {
class bb;
}
#endif
