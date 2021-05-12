#pragma once

#include <sing.h>

class Source final {
public:

    std::string getFullName() const;

    std::string path_;
    std::string base_;
    std::string ext_;
};

std::string fixBuild(bool *has_mods, const char *name, const std::vector<Source> &sources, const sing::map<std::string, int32_t> &srcbase2idx);
