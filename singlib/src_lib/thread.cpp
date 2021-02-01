#include "thread.h"

#ifdef _WIN32
#include <windows.h>
#include <synchapi.h>
#include <handleapi.h>
#else
#include <unistd.h>
#endif

namespace sing {

//
// toadd to the .h
//
// void *impl = nullptr; (Lock, Event)
// volatile unsigned int atomic = 0; (Atomic)
// void Run();  (Executer, public fun)
//

#ifdef _WIN32

Lock::~Lock()
{
    if (impl != nullptr) {
		DeleteCriticalSection((CRITICAL_SECTION*)impl);
        delete (CRITICAL_SECTION*)impl;
	}
}

bool Lock::init(const int32_t spincount)
{
    if (impl == nullptr) {
        CRITICAL_SECTION *c_sect = new CRITICAL_SECTION;
        if (c_sect == nullptr) return(false);
        InitializeCriticalSectionAndSpinCount(c_sect, spincount);
        impl = c_sect;
    }
}

void Lock::lock()
{
	if (impl != nullptr) {
		EnterCriticalSection((CRITICAL_SECTION*)impl);
	}
}

void Lock::unlock()
{
	if (impl != nullptr) {
		LeaveCriticalSection((CRITICAL_SECTION*)impl);
	}
}

bool Lock::trylock()
{
    if (impl != nullptr) {
		return(TryEnterCriticalSection((CRITICAL_SECTION*)impl) != 0);
	}
    return(false);
}

Event::~Event()
{
    if (impl != nullptr) {
        CloseHandle((HANDLE)impl);
    }
}

bool Event::init()
{
    if (impl == nullptr) {
        impl = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }
    return(impl != nullptr);
}

void Event::wait()
{
    if (impl != nullptr) {
        WaitForSingleObject((HANDLE)impl, INFINITE);
    }
}

void Event::signal()
{
    if (impl != nullptr) {
        SetEvent((HANDLE)impl);
    }
}

void Atomic::set(const int32_t value)
{
    atomic = (unsigned)value;
}

int32_t Atomic::get() const
{
    return((int32_t)atomic);
}

int32_t Atomic::inc()
{
    return((int32_t)InterlockedIncrement(&atomic));
}

int32_t Atomic::dec()
{
    return((int32_t)InterlockedDecrement(&atomic));
}

static DWORD WINAPI fnStub(void *lpParameter)
{
    ((void (*)())lpParameter)();
}

void runFn(void (*torun)(), const int32_t stack_size)
{
    ::CreateThread(nullptr, std::max(stack_size, 4096), fnStub, (void*)torun, 0, nullptr);
}

static DWORD WINAPI runnableStub(_In_ LPVOID lpParameter)
{
    std::shared_ptr<Runnable> *extra_ptr = (std::shared_ptr<Runnable>*)lpParameter;
    (*extra_ptr)->work(); 
    delete extra_ptr;
}

void run(const std::shared_ptr<Runnable> torun, const int32_t stack_size)
{
    // triky: we create a new pointer to increment runnable refcount and keep it alive.
    auto extra_ptr = new std::shared_ptr<Runnable>(torun);
    CreateThread(nullptr, std::max(stack_size, 4096), runnableStub, extra_ptr, 0, NULL);
}

static DWORD executerStub(void *pParam)
{
    Executer *ex = (Executer*)pParam;
    ex->Run();
    return(0);
}

static void executerStart(int32_t stack_size, Executer *ex)
{
    CreateThread(nullptr, std::max(stack_size, 4096), executerStub, ex, 0, NULL);
}

static DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    
    DWORD i;
    
    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest)?1:0);
        bitTest >>= 1;
    }

    return bitSetCount;
}

int32_t numCores()
{
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
	DWORD returnLength = 0;
	bool done = false;

	while (!done) {
		if (FALSE == GetLogicalProcessorInformation(buffer, &returnLength))	{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				if (buffer != nullptr) {
					free(buffer);
                }
				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
				if (nullptr == buffer) {
					return 1;
                }
			} else {
				return 1;
            }
		} else {
			done = true;
		}
	}
	
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
	DWORD byteOffset = 0;
	int processorCoreCount = 0;
	int logicalProcessorCount = 0;

	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
		if (ptr->Relationship == RelationProcessorCore) {
			processorCoreCount++;

			// A hyperthreaded core supplies more than one logical processor.
			logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	free(buffer);

	return logicalProcessorCount;
}

#else

struct LockImpl {
    pthread_mutex_t mutex;
    int             spincount;
};

Lock::~Lock()
{
    if (impl != nullptr) {
        pthread_mutex_destroy(&((LockImpl*)impl)->mutex);
        delete (LockImpl*)impl;
	}
}

bool Lock::init(const int32_t spincount)
{
    if (impl == nullptr) {
        LockImpl *c_mutex = new LockImpl;
        if (c_mutex == nullptr) return(false);
        if (pthread_mutex_init(&c_mutex->mutex, nullptr) != 0) {
            delete c_mutex;
            return(false);
        }
        c_mutex->spincount = spincount;
        impl = c_mutex;
    }
    return(impl != nullptr);
}

void Lock::lock()
{
	if (impl != nullptr) {
        LockImpl *c_mutex = (LockImpl*)impl;
        for (int tries = c_mutex->spincount; tries > 0; --tries) {
            if (pthread_mutex_trylock(&c_mutex->mutex) == 0) {
                return;
            }
        }        
        pthread_mutex_lock(&c_mutex->mutex);
	}
}

void Lock::unlock()
{
	if (impl != nullptr) {
		pthread_mutex_unlock(&((LockImpl*)impl)->mutex);
	}
}

bool Lock::trylock()
{
    if (impl != nullptr) {
		return(pthread_mutex_trylock(&((LockImpl*)impl)->mutex) == 0);
	}
    return(false);
}

struct EventImpl {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    bool            locked;
};

Event::~Event()
{
    if (impl != nullptr) {
        EventImpl *event = (EventImpl*)impl;
        pthread_mutex_destroy(&event->mutex);
        pthread_cond_destroy(&event->cond);
        delete event;
    }
}

bool Event::init()
{
    if (impl == nullptr) {
        EventImpl *c_mutex = new EventImpl;
        if (c_mutex == nullptr) return(false);
        if (pthread_mutex_init(&c_mutex->mutex, nullptr) != 0 ||
            pthread_cond_init(&c_mutex->cond, nullptr) != 0) {
            delete c_mutex;
            return(false);
        }
        c_mutex->locked = true;
        impl = c_mutex;
    }
    return(impl != nullptr);
}

void Event::wait()
{
    if (impl != nullptr) {
        EventImpl *event = (EventImpl*)impl;
        if (pthread_mutex_lock(&event->mutex) == 0) {
            while (event->locked)
            {
                pthread_cond_wait(&event->cond, &event->mutex);
            }
            event->locked = true;
            pthread_mutex_unlock(&event->mutex);
        }
    }
}

void Event::signal()
{
    if (impl != nullptr) {
        EventImpl *event = (EventImpl*)impl;
    	pthread_mutex_lock(&event->mutex);
	    event->locked = false;
	    pthread_cond_signal(&event->cond);
	    pthread_mutex_unlock(&event->mutex);
    }
}

void Atomic::set(const int32_t value)
{
    atomic = (unsigned)value;
}

int32_t Atomic::get() const
{
    return((int32_t)atomic);
}

int32_t Atomic::inc()
{
    return (int32_t)__sync_add_and_fetch ((volatile int *)&atomic,1);
}

int32_t Atomic::dec()
{
    return (int32_t)__sync_sub_and_fetch ((volatile int *)&atomic,1);
}

static void startThread(void *(*torun)(void*), void *arg, const int32_t stack_size)
{
    pthread_t temp;
    pthread_attr_t attr;

	if (pthread_attr_init(&attr) == 0) {
        pthread_attr_setstacksize (&attr, std::max(stack_size, 4096));
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        pthread_create(&temp, &attr, torun, arg);
        pthread_attr_destroy(&attr);
    }
}

static void *fnStub(void *lpParameter)
{
    ((void (*)())lpParameter)();
    return(nullptr);
}

void runFn(void (*torun)(), const int32_t stack_size)
{
    startThread(fnStub, torun, stack_size);
}

static void *runnableStub(void *lpParameter)
{
    std::shared_ptr<Runnable> *extra_ptr = (std::shared_ptr<Runnable>*)lpParameter;
    (*extra_ptr)->work(); 
    delete extra_ptr;
    return(nullptr);
}

void run(const std::shared_ptr<Runnable> torun, const int32_t stack_size)
{
    // triky: we create a new pointer to increment runnable refcount and keep it alive.
    auto extra_ptr = new std::shared_ptr<Runnable>(torun);
    startThread(runnableStub, extra_ptr, stack_size);
}

static void *executerStub(void *pParam)
{
    Executer *ex = (Executer*)pParam;
    ex->Run();
    return(nullptr);
}

static void executerStart(int32_t stack_size, Executer *ex)
{
    startThread(executerStub, ex, stack_size);
}

int32_t numCores()
{
	return((int32_t)sysconf( _SC_NPROCESSORS_ONLN));
}

#endif

static const int es_stopped = 0;
static const int es_running = 1;
static const int es_flushing = 2;
static const int es_stopping = 3;

Executer::Executer()
{
    status_.set(es_stopped);
}

Executer::~Executer()
{
    stop();
}

bool Executer::start(const int32_t queue_len, const int32_t stack_size)
{
    if (status_.get() != es_stopped) return(false);
    queue_.clear();
    queue_.resize(std::max(queue_len, 16));
    in_idx_ = 0;
    out_idx_ = 0;
    todo_idx_ = 0;
    done_count_.set(0);
    todo_count_.set(0);
    executerStart(stack_size, this);
    status_.set(es_running);
    return(true);
}

void Executer::stop()
{
    while (status_.get() == es_flushing) {
        insertion_.signal();
        stopped_.wait();
    }
    if (status_.get() == es_running) {
        status_.set(es_stopping);
    }
    while (status_.get() == es_stopping) {
        insertion_.signal();
        stopped_.wait();
    }
    queue_.clear(); // destroy the smart pointers to free the Runnables
}

void Executer::flush()
{
    if (status_.get() == es_running) {
        status_.set(es_flushing);
        while (status_.get() == es_flushing) {
            insertion_.signal();
            stopped_.wait();
        }
        while (getRunnable() != nullptr);
    }
}

void Executer::startFlushing()
{
    if (status_.get() == es_running) {
        status_.set(es_flushing);
        insertion_.signal();
    }
}

void Executer::setEvent(std::shared_ptr<Event> ev)
{
    ready_ = ev;
}

bool Executer::enqueue(const std::shared_ptr<Runnable> torun)
{
    if (torun == nullptr || status_.get() != es_running) return(false);
    if (done_count_.get() + todo_count_.get() >= queue_.size()) {
        return(false); // no room
    }
    queue_[in_idx_] = torun;
    if (++in_idx_ >= queue_.size()) in_idx_ = 0;
    todo_count_.inc();
    insertion_.signal();
    return(true);
}

std::shared_ptr<Runnable> Executer::getRunnable(bool blocking)
{
    if (status_.get() != es_running) return(nullptr);
    if (done_count_.get() <= 0) {
        if (!blocking) return(nullptr);
        if (ready_ != nullptr) {
            while (done_count_.get() <= 0) {
                (*ready_).wait();
            }
        } else {
            if (def_ready_ == nullptr) {
                def_ready_ = std::make_shared<Event>();    
                if (def_ready_ == nullptr) return(nullptr);            
            }
            ready_ = def_ready_;
            while (done_count_.get() <= 0) {
                (*ready_).wait();
            }
            ready_ = nullptr;
        }
    }
    std::shared_ptr<Runnable> retvalue = queue_[out_idx_];
    queue_[out_idx_] = nullptr;     // dont' extend the life of the object through the smart pointers.
    if (++out_idx_ >= queue_.size()) out_idx_ = 0;
    done_count_.dec();
    return(retvalue);
}

bool Executer::isFlushing() const
{
    return(status_.get() == es_flushing);
}

int32_t Executer::numQueued() const
{
    return(done_count_.get() + todo_count_.get());
}

int32_t Executer::numDone() const
{
    return(done_count_.get());
}

void Executer::Run()
{
    while (status_.get() != es_stopping) {

        // wait for new jobs
        insertion_.wait();

        // process or flush all the waiting jobs
        while (todo_count_.get() > 0) {
            if (status_.get() == es_running) {
                queue_[todo_idx_]->work();
            }
            if (++todo_idx_ >= queue_.size()) todo_idx_ = 0;
            done_count_.inc();
            todo_count_.dec();
            if (ready_ != nullptr && status_.get() == es_running) {
                (*ready_).signal();
            }
        }

        // signal that flushing is done (if we are here, all the jobs are done)
        if (status_.get() == es_flushing) {
            status_.set(es_running);
            stopped_.signal();
        }
    }
    status_.set(es_stopped);
    stopped_.signal();
}

}   // namespace