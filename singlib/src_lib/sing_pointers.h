#ifndef SING_POINTERS_H_
#define SING_POINTERS_H_

namespace sing {

//
// usage: ptr<T> name = new wrapper<T>;
//
// *name allows to access T.
//

//
// First we define wrapper: it wraps the pointed object, the reference counter and the weak pointer's list.
// (week pointers need to be set to null on deletion).
//
// Implementation needs the wptr definition and is downstream.
//

// forward declarations
template<class T> class wptr_base;
template<class T> class ptr;
template<class T> class wptr;

template<class T>
class wrapper {
    T               wrapped_;
    int32_t         counter_;
    wptr_base<T>    *weaks_;

public:
    wrapper() : counter_(0), weaks_(nullptr) {}
    wrapper(const T &initer) : wrapped_(initer), counter_(0), weaks_(nullptr) {}
    wrapper(const std::initializer_list<T> &list) : wrapped_(list), counter_(0), weaks_(nullptr) {}
    //wrapper(SING_SIZE_T size, std::initializer_list<T> list) : wrapped_(size, list), counter_(0), weaks_(nullptr) {}
    ~wrapper();

    // weak pointers support
    void ptr_register(wptr_base<T> *ptr);
    void ptr_unregister(wptr_base<T> *ptr);

    // counted pointers support
    // NOTE: this needs synchronization
    void inc_ref() { ++counter_; }
    int32_t dec_ref() { return(--counter_); }

    T &obj_ref() { return(wrapped_); }
    const T &obj_cref() { return(wrapped_); }
};

//
// this is the weak ptr base (from here derive wptr and cwptr).
// on changes registers/deregisters in the wrapper list.
// has accessors for list operations and is friend to wrapper (to let it access next_ and prev_)
//

template<class T>
class wptr_base {
protected:
    wrapper<T>      *pointed_;          // pointed objs must be wrapped by the wrapper !
    wptr_base<T>    *next_;             // chain of wptr's pointing to wrapper. They need to be nullified if wrapper dies.
    wptr_base<T>    *prev_;

    // utility
    void set(wrapper<T> *value) {
        if (pointed_ != nullptr) pointed_->ptr_unregister(this);
        pointed_ = value;
        if (pointed_ != nullptr) pointed_->ptr_register(this);
    }

public:
    wptr_base() : pointed_(nullptr), next_(nullptr), prev_(nullptr) {}
    ~wptr_base() { if (pointed_ != nullptr) pointed_->ptr_unregister(this); }

    // called when wrapper dies.
    void reset() {
        pointed_ = nullptr;
        next_ = nullptr;
        prev_ = nullptr;
    }
    wptr_base<T> *get_next() {
        return(next_);
    }

    // for assignments
    wrapper<T> *get_wrapper() const { return(pointed_); }

    friend wrapper<T>;      // must be able to link/unlink
};

//
// this is the ptr base (from here derive wptr and cwptr).
// on changes increments/decrements the wrapper ref_count, if needed deletes the wrapper
//

template<class T>
class ptr_base {
protected:
    wrapper<T>   *pointed_;

    inline void dec_cnt() {
        if (pointed_ != nullptr && pointed_->dec_ref() <= 0) {
            delete pointed_;
        }
    }

    inline void inc_cnt() {
        if (pointed_ != nullptr) {
            pointed_->inc_ref();
        }
    }

    void set(wrapper<T> *value) {
        dec_cnt();
        pointed_ = value;
        inc_cnt();
    }

public:

    // constructors
    ptr_base() : pointed_(nullptr) {}                               // default
    ptr_base(wrapper<T> *value) : pointed_(value) { inc_cnt(); }    // with pointed wrapper

    // destructor
    ~ptr_base() { dec_cnt(); }

    // for assignments
    wrapper<T> *get_wrapper() const { return(pointed_); }
};

//
// implementation of the list of weak pointers in wrapper_
// NOTE: this needs synchronization, also this needs to be synchronized with accesses from weak pointers !!
//

template<class T> 
void wrapper<T>::ptr_register(wptr_base<T> *ptr) {
    ptr->next_ = weaks_;
    ptr->prev_ = nullptr;
    if (weaks_ != nullptr) {
        weaks_->prev_ = ptr;
    }
    weaks_ = ptr;
}

template<class T>
void wrapper<T>::ptr_unregister(wptr_base<T> *ptr) {
    if (ptr->next_ != nullptr) {
        ptr->next_->prev_ = ptr->prev_;
    }
    if (ptr->prev_ != nullptr) {
        ptr->prev_->next_ = ptr->next_;
    } else {
        weaks_ = ptr->next_;
    }
}

template<class T>
wrapper<T>::~wrapper() {
    wptr_base<T> *scan, *next;
    for (scan = weaks_; scan != nullptr; scan = next) {
        next = scan->get_next();
        scan->reset();
    }
}

//
// the 4 pointer types. 
// they just differ by the allowed input (non const versions only allow non const inputs)
// and the returned refrences/pointers (const for const versions).
//
template<class T>
class wptr : public wptr_base<T> {
public:
    wptr() {}
    void operator=(wrapper<T> *value) { set(value); }   // need it for null assignments

    // copy construction
    wptr(const wptr<T> &right) {
        pointed_ = right.get_wrapper();
        if (pointed_ != nullptr) pointed_->ptr_register(this);
    }
    wptr(const ptr<T> &right) {
        pointed_ = right.get_wrapper();
        if (pointed_ != nullptr) pointed_->ptr_register(this);
    }

    // assignments
    void operator=(const wptr<T> &right) {
        set(right.get_wrapper());
    }
    void operator=(const ptr<T> &right) {
        set(right.get_wrapper());
    }

    // dereference
    T& operator*() const { return(pointed_->obj_ref()); }             // * operator
    operator T*() const { return(&pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};

template<class T>
class cwptr : public wptr_base<T> {
public:
    cwptr() {}
    void operator=(wrapper<T> *value) { set(value); }   // need it for null assignments

    // copy construction
    cwptr(const cwptr<T> &right) {
        pointed_ = right.get_wrapper();
        if (pointed_ != nullptr) pointed_->ptr_register(this);
    }
    cwptr(const wptr<T> &right) {
        pointed_ = right.get_wrapper();
        if (pointed_ != nullptr) pointed_->ptr_register(this);
    }
    cwptr(const ptr_base<T> &right) {
        pointed_ = right.get_wrapper();
        if (pointed_ != nullptr) pointed_->ptr_register(this);
    }

    // assignments
    void operator=(const cwptr<T> &right) {
        set(right.get_wrapper());
    }
    void operator=(const wptr<T> &right) {
        set(right.get_wrapper());
    }
    void operator=(const ptr_base<T> &right) {
        set(right.get_wrapper());
    }

    // dereference
    const T& operator*() const { return(pointed_->obj_ref()); }             // * operator
    operator const T*() const { return(&pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};

template<class T>
class ptr : public ptr_base<T> {
public:
    ptr() {}
    ptr(wrapper<T> *value) : ptr_base(value) {}         // with pointed wrapper
    void operator=(wrapper<T> *value) { set(value); }

    // copy construction
    ptr(const wptr<T> &right) {
        pointed_ = right.get_wrapper();
        inc_cnt();
    }
    ptr(const ptr<T> &right) {
        pointed_ = right.get_wrapper();
        inc_cnt();
    }

    // assignments
    void operator=(const wptr<T> &right) {
        set(right.get_wrapper());
    }
    void operator=(const ptr<T> &right) {
        set(right.get_wrapper());
    }

    // move assignment (swaps the pointers and keeps the refcounters unchanged, 'other' is supposed to die)
    void operator=(ptr<T> &&right) {
        wrapper<T> *tmp = right.pointed_;
        right.pointed_ = pointed_;
        pointed_ = tmp;
    }

    // dereference
    T& operator*() const { return(pointed_->obj_ref()); }             // * operator
    operator T*() const { return(&pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};

template<class T>
class cptr : public ptr_base<T> {
public:
    cptr() {}
    cptr(wrapper<T> *value) : ptr_base(value) {}     // with pointed wrapper
    void operator=(wrapper<T> *value) { set(value); }

    // copy construction
    cptr(const cptr<T> &right) {
        pointed_ = right.get_wrapper();
        inc_cnt();
    }
    cptr(const ptr<T> &right) {
        pointed_ = right.get_wrapper();
        inc_cnt();
    }
    cptr(const wptr_base<T> &right) {
        pointed_ = right.get_wrapper();
        inc_cnt();
    }

    // assignments
    void operator=(const wptr_base<T> &right) {
        set(right.get_wrapper());
    }
    void operator=(const ptr<T> &right) {
        set(right.get_wrapper());
    }
    void operator=(const cptr<T> &right) {
        set(right.get_wrapper());
    }

    // move assignment
    void operator=(cptr<T> &&right) {
        wrapper<T> *tmp = right.pointed_;
        right.pointed_ = pointed_;
        pointed_ = tmp;
    }

    // dereference
    const T& operator*() const { return(pointed_->obj_ref()); }             // * operator
    operator const T*() const { return(&pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};


}   // namespace

#endif