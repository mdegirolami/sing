#include "sing.h"
#include <memory>

void AllStdPtrOperations(void);

class Base0 {
public:
    virtual ~Base0() {}
    virtual void *get__id() = 0;
    virtual void setone(void) = 0;
};

class Base1 {
public:
    virtual ~Base1() {}
    virtual void *get__id() = 0;
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
    virtual void *get__id() { return(&id__); }
    static char id__;
};

char Derived::id__;

void AllStdPtrOperations(void)
{
    // default creation
    std::shared_ptr<Derived> p00;
    std::shared_ptr<const Derived> p01;
    std::weak_ptr<Derived> p02;
    std::weak_ptr<const Derived> p03;
    std::shared_ptr<Base1> p04;
    std::shared_ptr<const Base1> p05;
    std::weak_ptr<Base1> p06;
    std::weak_ptr<const Base1> p07;
    
    // nullptr constructor (not of much use but in sing you are allowed a null explicit initialization of a pointer)
    std::shared_ptr<Derived> px0(nullptr);
    std::shared_ptr<const Derived> px1(nullptr);
    //std::weak_ptr<Derived> px2(nulptr);           // OPS !! Weak ptrs can't be inited with nullptr
    //std::weak_ptr<const Derived> px3(nullptr);

    // construction with std::make_shared
    std::shared_ptr<Derived> p10 = std::make_shared<Derived>();
    std::shared_ptr<const Derived> p11 = std::make_shared<const Derived>();
    {

        // copy construction from ptr
        std::shared_ptr<Derived> p20(p10);          
        std::shared_ptr<const Derived> p21(p10);
        std::weak_ptr<Derived> p22(p10);
        std::weak_ptr<const Derived> p23(p10);
        std::shared_ptr<Base1> p24(p10);
        std::shared_ptr<const Base1> p25(p10);
        std::weak_ptr<Base1> p26(p10);
        std::weak_ptr<const Base1> p27(p10);

        // p10 refcount is now 5

        // copy construction from cptr
        std::shared_ptr<const Derived> p31(p11);
        std::weak_ptr<const Derived> p33(p11);
        std::shared_ptr<const Base1> p35(p11);
        std::weak_ptr<const Base1> p37(p11);

        // p11 refcount is now 3

        // copy construction from wptr
        std::shared_ptr<Derived> p40(p22);              // OPS !!! Throws if p22 is null !!
        std::shared_ptr<const Derived> p41(p22);
        std::weak_ptr<Derived> p42(p22);
        std::weak_ptr<const Derived> p43(p22);
        std::shared_ptr<Base1> p44(p22);
        std::shared_ptr<const Base1> p45(p22);
        std::weak_ptr<Base1> p46(p22);
        std::weak_ptr<const Base1> p47(p22);

        // p10 refcount is now 9

        // copy construction from cwptr
        std::shared_ptr<const Derived> p51(p23);
        std::shared_ptr<const Derived> p53(p23);
        std::weak_ptr<const Base1> p55(p23);
        std::weak_ptr<const Base1> p57(p23);

        // p10 refcount is now 11

        // copy construction from iptr
        std::shared_ptr<Base1> p64(p24);
        std::shared_ptr<const Base1> p65(p24);
        std::weak_ptr<Base1> p66(p24);
        std::weak_ptr<const Base1> p67(p24);

        // p10 refcount is now 13

        // copy construction from icptr
        std::shared_ptr<const Base1> p75(p25);
        std::weak_ptr<const Base1> p77(p25);

        // p10 refcount is now 14

        // copy construction from iwptr
        std::shared_ptr<Base1> p84(p26);
        std::shared_ptr<const Base1> p85(p26);
        std::weak_ptr<Base1> p86(p26);
        std::weak_ptr<const Base1> p87(p26);

        // p10 refcount is now 16

        // copy construction from icwptr
        std::shared_ptr<const Base1> p95(p27);
        std::weak_ptr<const Base1> p97(p27);

        // p10 refcount is now 17

        // downcasts must cause error

        //std::shared_ptr<Derived> p60(p24);

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
    p00 = p02.lock();
    p01 = p02.lock();
    p02 = p02.lock();
    p03 = p02.lock();
    p04 = p02.lock();
    p05 = p02.lock();
    p06 = p02.lock();
    p07 = p02.lock();

    // p11 refcnt = 1
    // p10 refcnt = 5

    // from cwptr
    p01 = p03.lock();
    p03 = p03;
    p05 = p03.lock();
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
    p04 = p06.lock();
    p05 = p06.lock();
    p06 = p06;
    p07 = p06;

    // from icwptr
    p05 = p07.lock();
    p07 = p07;

    // convert to plain
    Derived *pobj = &*p00;
    const Derived *pcobj = &*p01;
    Base1 *pbobj = &*p04;
    const Base1 *pcbobj = &*p05;

    // dereference (write)
    (*p00).setone();
    (*p04).settwo();
    (*p02.lock()).setone(); // even from weak !!

    // dereference (read)
    (*p00).get();
    (*p01).get();
    (*p04).get();
    (*p05).get();

    // comparisons
    bool b0 = p00 == p00;
    b0 = p00 == p01;
    b0 = p00 == p02.lock();
    b0 = p00 == p03.lock();

    b0 = p01 == p00;
    b0 = p01 == p01;
    b0 = p01 == p02.lock();
    b0 = p01 == p03.lock();

    b0 = p02.lock() == p00;
    b0 = p02.lock() == p01;
    b0 = p02.lock() == p02.lock();
    b0 = p02.lock() == p03.lock();

    b0 = p03.lock() == p00;
    b0 = p03.lock() == p01;
    b0 = p03.lock() == p02.lock();
    b0 = p03.lock() == p03.lock();

    b0 = p04 == p04;
    b0 = p04 == p05;
    b0 = p04 == p06.lock();
    b0 = p04 == p07.lock();

    b0 = p05 == p04;
    b0 = p05 == p05;
    b0 = p05 == p06.lock();
    b0 = p05 == p07.lock();

    b0 = p06.lock() == p04;
    b0 = p06.lock() == p05;
    b0 = p06.lock() == p06.lock();
    b0 = p06.lock() == p07.lock();

    b0 = p07.lock() == p04;
    b0 = p07.lock() == p05;
    b0 = p07.lock() == p06.lock();
    b0 = p07.lock() == p07.lock();

    p10 = nullptr;
    p11 = nullptr;

    // Here for convenience: typeswitch operations
    Base1 &inparm = *p00;
    Base1 *outparm = &*p00;

    if (inparm.get__id() == &Derived::id__) {
        Derived *localname = (Derived *)&inparm;
        localname->setone();
    }
    if ((*outparm).get__id() == &Derived::id__) {
        Derived *localname = (Derived *)&*outparm;
        localname->setone();
    }
    if ((*p04).get__id() == &Derived::id__) {
        std::shared_ptr<Derived> localname(p04, (Derived*)p04.get());
        (*localname).setone();
    }

    // assign to null
    // note: the object originally pointed by p10 is deleted by a virtual base destructor !!
    p00 = nullptr;
    p01 = nullptr;
    //p02 = nullptr;
    //p03 = nullptr;
    p04 = nullptr;
    p05 = nullptr;
    //p06 = nullptr;
    //p07 = nullptr;
}