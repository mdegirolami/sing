#ifndef SING_BASE_POINTERS_H_
#define SING_BASE_POINTERS_H_

namespace sing {

class base {
    virtual ~base() = 0;
    virtual void *GetId(void) = 0;
};

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
//template<class T> class wptr_base;
//template<class T> class ptr;
//template<class T> class wptr;

template<class T>
class wrapper {
    int32_t counter_;
    T       wrapped_;

public:
    wrapper() : counter_(0) {}
    wrapper(const T &initer) : wrapped_(initer), counter_(0) {}
        wrapper(const std::initializer_list<T> &list) : wrapped_(list), counter_(0) {}
    //wrapper(SING_SIZE_T size, std::initializer_list<T> list) : wrapped_(size, list), counter_(0), weaks_(nullptr) {}
    //~wrapper();

    // counted pointers support
    // NOTE: this needs synchronization
    void inc_ref() {
        //_InterlockedIncrement(reinterpret_cast<volatile long *>(&counter_));
        ++counter_; 
    }
    int32_t dec_ref() {
        //return(_InterlockedDecrement(reinterpret_cast<volatile long *>(&counter_)));
        return(--counter_);
    }

    T &obj_ref() { return(wrapped_); }
    const T &obj_cref() { return(wrapped_); }
};

//
// weak ptr base (from here derive wptr and cwptr).
//

template<class T>
class wptr_base {
protected:
    wrapper<T>      *pointed_;          // pointed objs must be wrapped by the wrapper !

    // construction
    wptr_base() : pointed_(nullptr) {}
    wptr_base(wrapper<T> *value) : pointed_(value) {}

    // assignment
    void set(wrapper<T> *value) {
        pointed_ = value;
    }

public:
    // read back
    wrapper<T> *get_wrapper() const { return(pointed_); }
};

//
// this is the strong ptr base (from here derive wptr and cwptr).
// on changes increments/decrements the wrapper ref_count, if needed deletes the wrapper
//
template<class T>
class ptr_base {
protected:
    wrapper<T>   *pointed_;

    // constructors
    ptr_base() : pointed_(nullptr) {}                               // default
    ptr_base(wrapper<T> *value) : pointed_(value) { inc_cnt(); }    // with pointed wrapper

                                                                    // destruction
    ~ptr_base() { dec_cnt(); }

    // assignment
    void set(wrapper<T> *value) {
        dec_cnt();
        pointed_ = value;
        inc_cnt();
    }

    // for move assignment
    void swap(ptr_base<T> &right) {
        wrapper<T> *tmp = right.pointed_;
        right.pointed_ = pointed_;
        pointed_ = tmp;
    }

public:
    // read back
    wrapper<T> *get_wrapper() const { return(pointed_); }

private:
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
};

//
// pointers for interfaces (weak and strong)
//
template<class T>
class iwptr_base {
protected:
    wrapper<void*>  *pointed_;          // pointed objs must be wrapped by the wrapper !
    T               *ifptr_;

    // construction
    iwptr_base() : pointed_(nullptr), ifptr_(nullptr) {}
    iwptr_base(void *wrapper_ptr, T *obj) : pointed_((wrapper<void*>*)wrapper_ptr), ifptr_(obj) {}

    //template<class S>
    //iwptr_base(wrapper<void> *wrapper, T *obj) : pointed_(wrapper),  {
    //    ifptr_ = (T*)&value->obj_ref();
    //}

    // assignment
    void set(void *value, T *obj) {
        pointed_ = (wrapper<void*>*)value;
        ifptr_ = obj;
    }

    //template<class S>
    //void set(wrapper<S> *value) {
    //    pointed_ = value;
    //    ifptr_ = (T*)&value->obj_ref();
    //}

public:
    // read back
    wrapper<void*> *get_wrapper() const { return(pointed_); }
    T *get_wrapped() const { return(ifptr_); }
};

template<class T>
class iptr_base {
protected:
    wrapper<void*>  *pointed_;          // pointed objs must be wrapped by the wrapper !
    T               *ifptr_;

    // construction
    iptr_base() : pointed_(nullptr), ifptr_(nullptr) {}
    iptr_base(void *wrapper_ptr, T *obj) : pointed_((wrapper<void*>*)wrapper_ptr), ifptr_(obj) { inc_cnt(); }

    // destruction
    ~iptr_base() { dec_cnt(); }

    // assignment
    void set(void *value, T *obj) {
        dec_cnt();
        pointed_ = (wrapper<void*>*)value;
        ifptr_ = obj;
        inc_cnt();
    }

public:
    // read back
    wrapper<void*> *get_wrapper() const { return(pointed_); }
    T *get_wrapped() const { return(ifptr_); }

private:
    inline void dec_cnt() {
        if (pointed_ != nullptr && pointed_->dec_ref() <= 0) {
            ifptr_->~T();        // destruct the wrapped obj through a virtual destructor
            delete pointed_;    // destruct the wrapped and free the memory
        }
    }

    inline void inc_cnt() {
        if (pointed_ != nullptr) {
            pointed_->inc_ref();
        }
    }
};

} // namespace

#endif