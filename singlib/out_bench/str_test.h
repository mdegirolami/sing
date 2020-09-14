#pragma once

#include <sing.h>
#include "str.h"

class NumberSelector final : public sing::Selector {
public:
    virtual void *get__id() const override { return(&id__); };
    virtual bool isGood(const int32_t cp) const override;

    static char id__;
};

bool str_test();
