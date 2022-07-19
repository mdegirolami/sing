#include "singmake.h"

int main(int argc, const char *argv[])
{
    std::vector<std::string>    singarg;

    for (int ii = 0; ii < argc; ++ii) {
        singarg.push_back(argv[ii]);
    }
    return(singmain(singarg));
}