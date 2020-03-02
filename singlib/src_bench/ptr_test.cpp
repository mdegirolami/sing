#include "sing.h"
#include <memory>

void AllPtrOperations(void);

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
    sing::ptr<sing::dvect<int>> ax(new sing::wrapper<sing::dvect<int>>);    // var ax[*]int;
    *ax = {1, 2, 3};

    // check what happens on deletion
    sing::wptr<int[100]> weak3 = aa;
    sing::wptr<int[100]> weak_x = nullptr;

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
    //if (weak3 != nullptr) {
    //    printf("shouldn't pass here !!");
    //}

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
    //if (cweak3 != nullptr) {
    //    printf("shouldn't pass here !!");
    //}

    AllPtrOperations();
}

class Base0 {
public:
    virtual ~Base0() {}
    virtual void setone(void) = 0;
};

class Base1 {
public:
    virtual ~Base1() {}
    virtual void settwo(void) = 0;
    virtual int get(void) const = 0;
};

class Derived : public Base0, public Base1 {
public:
    virtual ~Derived() { val = 0; }
    virtual void setone(void) { val = 1; };
    virtual void settwo(void) { val = 2; };
    virtual int get(void) const { return(val); }
    int val;
};

void AllPtrOperations(void)
{
    // default creation
    sing::ptr<Derived> p00;
    sing::cptr<Derived> p01;
    sing::wptr<Derived> p02;
    sing::cwptr<Derived> p03;
    sing::iptr<Base1> p04;
    sing::icptr<Base1> p05;
    sing::iwptr<Base1> p06;
    sing::icwptr<Base1> p07;
    
    // nullptr constructor (not of much use but in sing you are allowed a null explicit initialization of a pointer)
    sing::ptr<Derived> px0(nullptr);
    sing::cptr<Derived> px1(nullptr);
    sing::wptr<Derived> px2(nullptr);
    sing::cwptr<Derived> px3(nullptr);
    sing::iptr<Base1> px4(nullptr);
    sing::icptr<Base1> px5(nullptr);
    sing::iwptr<Base1> px6(nullptr);
    sing::icwptr<Base1> px7(nullptr);

    // construction with wrapper pointer
    sing::ptr<Derived> p10(new sing::wrapper<Derived>);
    sing::cptr<Derived> p11(new sing::wrapper<Derived>);
    
    {

        // copy construction from ptr
        sing::ptr<Derived> p20(p10);
        sing::cptr<Derived> p21(p10);
        sing::wptr<Derived> p22(p10);
        sing::cwptr<Derived> p23(p10);
        sing::iptr<Base1> p24(p10);
        sing::icptr<Base1> p25(p10);
        sing::iwptr<Base1> p26(p10);
        sing::icwptr<Base1> p27(p10);

        // p10 refcount is now 5

        // copy construction from cptr
        sing::cptr<Derived> p31(p11);
        sing::cwptr<Derived> p33(p11);
        sing::icptr<Base1> p35(p11);
        sing::icwptr<Base1> p37(p11);

        // p11 refcount is now 3

        // copy construction from wptr
        sing::ptr<Derived> p40(p22);
        sing::cptr<Derived> p41(p22);
        sing::wptr<Derived> p42(p22);
        sing::cwptr<Derived> p43(p22);
        sing::iptr<Base1> p44(p22);
        sing::icptr<Base1> p45(p22);
        sing::iwptr<Base1> p46(p22);
        sing::icwptr<Base1> p47(p22);

        // p10 refcount is now 9

        // copy construction from cwptr
        sing::cptr<Derived> p51(p23);
        sing::cwptr<Derived> p53(p23);
        sing::icptr<Base1> p55(p23);
        sing::icwptr<Base1> p57(p23);

        // p10 refcount is now 11

        // copy construction from iptr
        sing::iptr<Base1> p64(p24);
        sing::icptr<Base1> p65(p24);
        sing::iwptr<Base1> p66(p24);
        sing::icwptr<Base1> p67(p24);

        // p10 refcount is now 13

        // copy construction from icptr
        sing::icptr<Base1> p75(p25);
        sing::icwptr<Base1> p77(p25);

        // p10 refcount is now 14

        // copy construction from iwptr
        sing::iptr<Base1> p84(p26);
        sing::icptr<Base1> p85(p26);
        sing::iwptr<Base1> p86(p26);
        sing::icwptr<Base1> p87(p26);

        // p10 refcount is now 16

        // copy construction from icwptr
        sing::icptr<Base1> p95(p27);
        sing::icwptr<Base1> p97(p27);

        // p10 refcount is now 17

        // THESE MUST CAUSE ERRORS
    
        // loosing constness

        //sing::ptr<Derived> p30(p11);
        //sing::wptr<Derived> p32(p11);
        //sing::iptr<Base1> p34(p11);
        //sing::iwptr<Base1> p36(p11);

        //sing::ptr<Derived> p50(p23);
        //sing::wptr<Derived> p52(p23);
        //sing::iptr<Base1> p54(p23);
        //sing::iwptr<Base1> p56(p23);

        //sing::iptr<Base1> p74(p25);
        //sing::iwptr<Base1> p76(p25);

        //sing::iptr<Base1> p94(p27);
        //sing::iwptr<Base1> p96(p27);

        // downcasts

        //sing::ptr<Derived> p60(p24);
        //sing::cptr<Derived> p61(p24);
        //sing::wptr<Derived> p62(p24);
        //sing::cwptr<Derived> p63(p24);

        //sing::ptr<Derived> p70(p25);
        //sing::cptr<Derived> p71(p25);
        //sing::wptr<Derived> p72(p25);
        //sing::cwptr<Derived> p73(p25);

        //sing::ptr<Derived> p80(p26);
        //sing::cptr<Derived> p81(p26);
        //sing::wptr<Derived> p82(p26);
        //sing::cwptr<Derived> p83(p26);

        //sing::ptr<Derived> p90(p27);
        //sing::cptr<Derived> p91(p27);
        //sing::wptr<Derived> p92(p27);
        //sing::cwptr<Derived> p93(p27);

    } // all refcounts back to 1 !!

    // ASSIGNMENTS

    // from ptr
    p00 = p10;
    p01 = p10;
    p02 = p10;
    p03 = p10;
    p04 = p10;
    p05 = p10;
    p06 = p10;
    p07 = p10;

    // refcnt = 5

    // from cptr
    p01 = p11;
    p03 = p11;
    p05 = p11;
    p07 = p11;

    // p11 refcnt = 3
    // p10 refcnt = 3

    // from wptr
    p00 = p02;
    p01 = p02;
    p02 = p02;
    p03 = p02;
    p04 = p02;
    p05 = p02;
    p06 = p02;
    p07 = p02;

    // p11 refcnt = 1
    // p10 refcnt = 5

    // from cwptr
    p01 = p03;
    p03 = p03;
    p05 = p03;
    p07 = p03;

    // from iptr
    p04 = p04;
    p05 = p04;
    p06 = p04;
    p07 = p04;

    // from icptr
    p05 = p05;
    p07 = p05;

    // from iwptr
    p04 = p06;
    p05 = p06;
    p06 = p06;
    p07 = p06;

    // from icwptr
    p05 = p07;
    p07 = p07;

    // THESE MUST CAUSE ERRORS

    // loosing constness

    //p00 = p11;
    //p02 = p11;
    //p04 = p11;
    //p06 = p11;

    //p00 = p03;
    //p02 = p03;
    //p04 = p03;
    //p06 = p03;

    //p04 = p05;
    //p06 = p05;
    //p04 = p07;
    //p06 = p07;

    // downcast

    //p00 = p04;
    //p01 = p04;
    //p02 = p04;
    //p03 = p04;

    //p00 = p05;
    //p01 = p05;
    //p02 = p05;
    //p03 = p05;

    //p00 = p06;
    //p01 = p06;
    //p02 = p06;
    //p03 = p06;

    //p00 = p07;
    //p01 = p07;
    //p02 = p07;
    //p03 = p07;


    // convert to plain
    Derived *pobj = p00;
    const Derived *pcobj = p01;
    pobj = p02;
    pcobj = p03;
    Base1 *pbobj = p04;
    const Base1 *pcbobj = p05;
    pbobj = p06;
    pcbobj = p07;

    // dereference (write)
    (*p00).setone();
    (*p02).setone();
    (*p04).settwo();
    (*p06).settwo();

    // ERRORS (constness !!)
    //(*p01).setone();
    //(*p03).setone();
    //(*p05).settwo();
    //(*p07).settwo();

    // dereference (read)
    (*p00).get();
    (*p01).get();
    (*p02).get();
    (*p03).get();
    (*p04).get();
    (*p05).get();
    (*p06).get();
    (*p07).get();

    // comparisons
    bool b0 = p00 == p00;
    b0 = p00 == p01;
    b0 = p00 == p02;
    b0 = p00 == p03;

    b0 = p01 == p00;
    b0 = p01 == p01;
    b0 = p01 == p02;
    b0 = p01 == p03;

    b0 = p02 == p00;
    b0 = p02 == p01;
    b0 = p02 == p02;
    b0 = p02 == p03;

    b0 = p03 == p00;
    b0 = p03 == p01;
    b0 = p03 == p02;
    b0 = p03 == p03;

    b0 = p04 == p04;
    b0 = p04 == p05;
    b0 = p04 == p06;
    b0 = p04 == p07;

    b0 = p05 == p04;
    b0 = p05 == p05;
    b0 = p05 == p06;
    b0 = p05 == p07;

    b0 = p06 == p04;
    b0 = p06 == p05;
    b0 = p06 == p06;
    b0 = p06 == p07;

    b0 = p07 == p04;
    b0 = p07 == p05;
    b0 = p07 == p06;
    b0 = p07 == p07;

    p10 = nullptr;
    p11 = nullptr;

    // assign to null
    // note: the object originally pointed by p10 is deleted by a virtual base destructor !!
    p00 = nullptr;
    p01 = nullptr;
    p02 = nullptr;
    p03 = nullptr;
    p04 = nullptr;
    p05 = nullptr;
    p06 = nullptr;
    p07 = nullptr;
}
