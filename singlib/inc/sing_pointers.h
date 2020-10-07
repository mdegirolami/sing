#pragma once

#include "sing_base_pointers.h"

namespace sing {

template<class T> class ptr;
template<class T> class iptr;

//
// the 4 pointer types. 
// they just differ by the allowed input (non const versions only allow non const inputs)
// and the returned refrences/pointers (const for const versions).
//
////////////////////////////////////
//
// Weak, concrete target
//
////////////////////////////////////
template<class T>
class wptr : public wptr_base<T> {
public:
    // construction
    wptr() {}

    // copy construction
    wptr(const wptr<T> &right) : wptr_base<T> (right.get_wrapper()) {}
    wptr(const ptr<T> &right) : wptr_base<T>(right.get_wrapper()) {}
    wptr(const std::nullptr_t &right) {}

    // assignments
    void operator=(const wptr<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(const ptr<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(wrapper<T> *value) { this->set(value); } // need it for null assignments

    // dereference
    T& operator*() const { return(this->pointed_->obj_ref()); }             // * operator
    operator T*() const { return(&this->pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};

template<class T>
class cwptr : public wptr_base<T> {
public:
    // special members (all 3 explicitly declared to avoid surprises)
    cwptr() {}
    cwptr(const cwptr<T> &right) : wptr_base<T>(right.get_wrapper()) {}
    void operator=(const cwptr<T> &right) {
        this->set(right.get_wrapper());
    }

    // copy construction
    cwptr(const wptr_base<T> &right) : wptr_base<T>(right.get_wrapper()) {}
    cwptr(const ptr_base<T> &right) : wptr_base<T>(right.get_wrapper()) {}
    cwptr(const std::nullptr_t &right) {}

    // assignments
    void operator=(const wptr_base<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(const ptr_base<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(wrapper<T> *value) { this->set(value); }   // need it for null assignments

    // dereference
    const T& operator*() const { return(this->pointed_->obj_ref()); }             // * operator
    operator const T*() const { return(&this->pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};

////////////////////////////////////
//
// Strong, concrete target
//
////////////////////////////////////
template<class T>
class ptr : public ptr_base<T> {
public:
    ptr() {}
    ptr(wrapper<T> *value) : ptr_base<T>(value) {}         // with pointed wrapper

    // copy construction
    ptr(const wptr<T> &right) : ptr_base<T>(right.get_wrapper()) {}
    ptr(const ptr<T> &right) : ptr_base<T>(right.get_wrapper()) {}
    ptr(const std::nullptr_t &right) {}

    // assignments
    void operator=(const wptr<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(const ptr<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(wrapper<T> *value) { this->set(value); }

    // move assignment (swaps the pointers and keeps the refcounters unchanged, 'other' is supposed to die)
    // if we were pointing to something the 'right' pointer will dec. the refcount on destruction
    void operator=(ptr<T> &&right) {
        this->swap(right);
    }

    // dereference
    T& operator*() const { return(this->pointed_->obj_ref()); }             // * operator
    operator T*() const { return(&this->pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};

template<class T>
class cptr : public ptr_base<T> {
public:
    // special members (all 3 explicitly declared to avoid surprises)
    cptr() {}
    cptr(const cptr<T> &right) : ptr_base<T>(right.get_wrapper()) {}
    void operator=(const cptr<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(cptr<T> &&right) {
        wrapper<T> *tmp = right.pointed_;
        right.pointed_ = this->pointed_;
        this->pointed_ = tmp;
    }
    cptr(wrapper<T> *value) : ptr_base<T>(value) {}     // with pointed wrapper

    // copy construction
    cptr(const ptr_base<T> &right) : ptr_base<T>(right.get_wrapper()) {}
    cptr(const wptr_base<T> &right) : ptr_base<T>(right.get_wrapper()) {}
    cptr(const std::nullptr_t &right) {}

    // assignments
    void operator=(const wptr_base<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(const ptr_base<T> &right) {
        this->set(right.get_wrapper());
    }
    void operator=(wrapper<T> *value) { this->set(value); }

    // move assignment
    void operator=(ptr_base<T> &&right) {
        this->swap(right);
    }

    // dereference
    const T& operator*() const { return(this->pointed_->obj_ref()); }             // * operator
    operator const T*() const { return(&this->pointed_->obj_ref()); }             // automatic conversion to a standard pointer
};

////////////////////////////////////
//
// Interface Weak
//
////////////////////////////////////
template<class T>
class iwptr : public iwptr_base<T> {
public:
    // special members
    iwptr() {}
    iwptr(const iwptr<T> &right) : iwptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    void operator=(const iwptr<T> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }

    // copy construction
    template<class S>
    iwptr(const wptr<S> &right) : iwptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    iwptr(const ptr<S> &right) : iwptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    iwptr(const iwptr<S> &right) : iwptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    template<class S>
    iwptr(const iptr<S> &right) : iwptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    iwptr(const std::nullptr_t &right) {}


    // assignments
    template<class S>
    void operator=(const wptr<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const ptr<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const iwptr<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    template<class S>
    void operator=(const iptr<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    void operator=(void *value) { this->set(nullptr, nullptr); } // need it for null assignments

    // dereference
    T& operator*() const { return(*this->get_wrapped()); }    // * operator
    operator T*() const { return(this->get_wrapped()); }      // automatic conversion to a standard pointer
};

template<class T>
class icwptr : public iwptr_base<T> {
public:
    // construction
    icwptr() {}
    icwptr(const icwptr<T> &right) : iwptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    void operator=(const icwptr<T> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }

    // copy construction
    template<class S>
    icwptr(const wptr_base<S> &right) : iwptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    icwptr(const ptr_base<S> &right) : iwptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    icwptr(const iwptr_base<S> &right) : iwptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    template<class S>
    icwptr(const iptr_base<S> &right) : iwptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    icwptr(const std::nullptr_t &right) {}

    // assignments
    template<class S>
    void operator=(const wptr_base<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const ptr_base<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const iwptr_base<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    template<class S>
    void operator=(const iptr_base<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    void operator=(void *value) { this->set(nullptr, nullptr); } // need it for null assignments

                                                                 // dereference
    const T& operator*() const { return(*this->get_wrapped()); }    // * operator
    operator const T*() const { return(this->get_wrapped()); }      // automatic conversion to a standard pointer
};

////////////////////////////////////
//
// Strong, concrete target
//
////////////////////////////////////
template<class T>
class iptr : public iptr_base<T> {
public:
    iptr() {}
    iptr(const iptr<T> &right) : iptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    void operator=(const iptr<T> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }

    // copy construction
    template<class S>
    iptr(const wptr<S> &right) : iptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    iptr(const ptr<S> &right) : iptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    iptr(const iwptr<S> &right) : iptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    template<class S>
    iptr(const iptr<S> &right) : iptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    iptr(const std::nullptr_t &right) {}

    // assignments
    template<class S>
    void operator=(const wptr<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const ptr<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const iwptr<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    template<class S>
    void operator=(const iptr<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    void operator=(void *value) { this->set(nullptr, nullptr); } // need it for null assignments

    // dereference
    T& operator*() const { return(*this->get_wrapped()); }    // * operator
    operator T*() const { return(this->get_wrapped()); }      // automatic conversion to a standard pointer
};

template<class T>
class icptr : public iptr_base<T> {
public:
    // construction
    icptr() {}
    icptr(const icptr<T> &right) : iptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    void operator=(const icptr<T> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }

    // copy construction
    template<class S>
    icptr(const wptr_base<S> &right) : iptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    icptr(const ptr_base<S> &right) : iptr_base<T>(right.get_wrapper(), &right.get_wrapper()->obj_ref()) {}
    template<class S>
    icptr(const iwptr_base<S> &right) : iptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    template<class S>
    icptr(const iptr_base<S> &right) : iptr_base<T>(right.get_wrapper(), right.get_wrapped()) {}
    icptr(const std::nullptr_t &right) {}

    // assignments
    template<class S>
    void operator=(const wptr_base<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const ptr_base<S> &right) {
        this->set(right.get_wrapper(), &right.get_wrapper()->obj_ref());
    }
    template<class S>
    void operator=(const iwptr_base<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    template<class S>
    void operator=(const iptr_base<S> &right) {
        this->set(right.get_wrapper(), right.get_wrapped());
    }
    void operator=(void *value) { this->set(nullptr, nullptr); } // need it for null assignments

    // dereference
    const T& operator*() const { return(*this->get_wrapped()); }    // * operator
    operator const T*() const { return(this->get_wrapped()); }      // automatic conversion to a standard pointer
};

}   // namespace
