#include "updater.h"
#include "sio.h"

int32_t updater(const std::vector<std::string> &argv)
{
    sing::print("hello world !!!\r\n");
    sing::print("\r\nPress any key to exit...");
    sing::kbdGet();
    return (0);
}
