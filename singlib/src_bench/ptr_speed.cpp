#include "sing.h"
#include <memory>
#include <time.h>

class singnode {
public:
    sing::ptr<singnode> left;
    sing::ptr<singnode> right;
};

sing::ptr<singnode> BuidSingTree(int size)
{
    sing::ptr<singnode> node = new sing::wrapper<singnode>;
    if (size > 1) {
        (*node).left = BuidSingTree(size - 1);
        (*node).right = BuidSingTree(size - 1);
    }
    return(node);
}

class stdnode {
public:
    std::shared_ptr<stdnode> left;
    std::shared_ptr<stdnode> right;
};

std::shared_ptr<stdnode> BuidStdTree(int size)
{
    std::shared_ptr<stdnode> node(new stdnode);
    if (size > 1) {
        (*node).left = BuidStdTree(size - 1);
        (*node).right = BuidStdTree(size - 1);
    }
    return(node);
}

class my_if {
public:
    virtual int kkk() = 0;
};

class my_if2 {
public:
    virtual int kkk2() = 0;
};

class concrete : public my_if2, my_if {
public:
    virtual int kkk() { return(0); }
    virtual int kkk2() { return(10); }
};

template<class DstT, class SrcT>
void upcast(sing::ptr<DstT> dst, sing::ptr<SrcT>)
{
    dst->pointed_ = src->pointed_;
}

void test_ptr_speed()
{
    clock_t start = clock();
    for (int ii = 0; ii < 3; ++ii) {
        BuidSingTree(20);   // build a tree of 2^20-1 nodes, then deletes it (when the pointer exits its scope !)
    }
    printf("\ntime = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
    for (int ii = 0; ii < 3; ++ii) {
        BuidStdTree(20);   // build a tree of 2^20-1 nodes, then deletes it (when the pointer exits its scope !)
    }
    printf("\ntime = %d", (clock() - start) * 1000 / CLOCKS_PER_SEC);
    getchar();
}
