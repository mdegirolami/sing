#include "sing.h"
#include <memory>

/*
sing::vector<int> *tmp__vector = new sing::vector<int>({ 0,1,2 });
sing::ptr<sing::vector<int>> ax(new sing::wrapper<sing::vector<int>>({ 0,1,2 }));    // var aa [100]int;
sing::ptr<int[100]> aa(new sing::wrapper<int[100]>);    // var aa [100]int;
*/


sing::ptr<int[100]> Create(void) {                          // func Create() *[100]int

    /* alternate                            
    sing::ptr<int[100]> aa = new sing::wrapper<int[100]>;   // var aa [100]int;
    */
    sing::ptr<int[100]> aa(new sing::wrapper<int[100]>);    // var aa [100]int;

    // inited and dyna versions
    sing::ptr<int> single(new sing::wrapper<int>(3));
    sing::ptr<sing::vector<int>> ax(new sing::wrapper<sing::vector<int>>);    // var ax[*]int;
    *ax = {1, 2, 3};

    // check what happens on deletion
    sing::wptr<int[100]> weak3 = aa;   

    return(aa);                                             // return &a;
}

sing::cptr<int[100]> Dummy(const sing::ptr<int[100]> &in)
{
    return(in);
}

void ptrtest(void)
{
    sing::ptr<int[100]> ptr1(Create());    // copy constructor
    sing::ptr<int[100]> ptr2, ptr3;
    sing::wptr<int[100]> weak1, weak2;
    ptr2 = ptr1;                            // assignment
    ptr3 = ptr2;
    ptr1 = nullptr;
    ptr3 = nullptr;
    weak1 = ptr2;                       // assignment from pointer and first link
    weak2 = weak1;                      // assignment from weak ptr and subsequent link
    sing::wptr<int[100]> weak3(ptr2);   // copy constructor from ptr
    weak2 = nullptr;                    // unlink, not first or last
    weak1 = nullptr;                    // unlink last
    weak2 = ptr2;
    weak2 = nullptr;                    // unlink first
    (*weak3)[1] = 10;                   // dereference operator
    (*ptr2)[2] = 5;
    int(*test)[100] = weak3;            // convert to std pointer
    int(*test2)[100] = ptr2;
    sing::wptr<int[100]> weak4(weak3);  // copy constructor from weak
    sing::ptr<int[100]> ptr4(weak3);
    ptr4 = ptr1;
    ptr4 = weak3;
    ptr4 = nullptr;
    ptr2 = nullptr;                     // deletion
    if (weak3 != nullptr) {
        printf("shouldn't pass here !!");
    }

    // consts
    ptr1 = Create();    // assignment
    weak1 = ptr1;
    //sing::cptr<int[100]> cptr1(Create());     // copy constructor
    sing::cptr<int[100]> cptr1 = weak1;         // assignment
    sing::cptr<int[100]> cptr2, cptr3;
    sing::cwptr<int[100]> cweak1, cweak2;
    cptr2 = Create();                         // test move assignment ptr->cptr
    cptr2 = Dummy(weak1);
    cptr2 = cptr1;                            // assignment
    cptr3 = ptr1;
    cptr1 = nullptr;
    cptr3 = nullptr;
    cweak1 = cptr2;                       // assignment from pointer and first link
    cweak2 = cweak1;                      // assignment from weak ptr and subsequent link
    sing::cwptr<int[100]> cweak3(cptr2);   // copy constructor from ptr
    cweak2 = nullptr;                    // unlink, not first or last
    cweak1 = nullptr;                    // unlink last
    cweak2 = cptr2;
    cweak2 = nullptr;                    // unlink first
    cweak2 = weak1;
    int v0 = (*cweak3)[1];               // dereference operator
    int v1 = (*cptr2)[2];
    const int(*ctest)[100] = cweak3;            // convert to std pointer
    const int(*ctest2)[100] = cptr2;
    //sing::cwptr<int[100]> cweak4= {cweak3};  // copy constructor from weak
    sing::cwptr<int[100]> cweak4(cweak3);  // copy constructor from weak
    sing::cptr<int[100]> cptr4(cweak3);
    sing::cptr<int[100]> cptr5(cptr4);
    sing::cwptr<int[100]> cweak5(weak1);
    cptr4 = cptr1;
    cptr4 = cweak3;
    cweak1 = ptr1;
    ptr1 = nullptr;
    cptr4 = nullptr;
    cptr5 = nullptr;
    cptr2 = nullptr;                     // deletion
    if (cweak3 != nullptr) {
        printf("shouldn't pass here !!");
    }
}
