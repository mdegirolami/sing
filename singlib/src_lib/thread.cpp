#include "thread.h"

#ifdef _WIN32
#include <windows.h>
#include <synchapi.h>
#include <handleapi.h>
#else
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
    sing::iptr<Runnable> *extra_ptr = (sing::iptr<Runnable>*)lpParameter;
    Runnable *torun = *extra_ptr; 
    torun->work();
    delete extra_ptr;
}

void run(const sing::iptr<Runnable> torun, const int32_t stack_size)
{
    // triky: we create a new pointer to increment runnable refcount and keep it alive.
    auto extra_ptr = new sing::iptr<Runnable>(torun);
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
    CreateThread(nullptr, std::max(stack_size, 4096), runnableStub, ex, 0, NULL);
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

#endif

Executer::Executer()
{
    status_ = ExecuterStatus::stopped;
}

Executer::~Executer()
{
    stop();
}

bool Executer::start(const int32_t queue_len, const int32_t stack_size)
{
    if (status_ != ExecuterStatus::stopped) return(false);
    queue_.clear();
    queue_.resize(std::max(queue_len, 16));
    in_idx_ = 0;
    out_idx_ = 0;
    todo_idx_ = 0;
    done_count_.set(0);
    todo_count_.set(0);
    executerStart(stack_size, this);
    status_ = ExecuterStatus::running;
    return(true);
}

void Executer::stop()
{
    while (status_ == ExecuterStatus::flushing) {
        insertion_.signal();
        stopped_.wait();
    }
    if (status_ == ExecuterStatus::running) {
        status_ = ExecuterStatus::stopping;
    }
    while (status_ == ExecuterStatus::stopping) {
        insertion_.signal();
        stopped_.wait();
    }
    queue_.clear(); // destroy the smart pointers to free the Runnables
}

void Executer::flush()
{
    if (status_ == ExecuterStatus::running) {
        status_ = ExecuterStatus::flushing;
        while (status_ == ExecuterStatus::flushing) {
            insertion_.signal();
            stopped_.wait();
        }
        while (getRunnable() != nullptr);
    }
}

void Executer::startFlushing()
{
    if (status_ == ExecuterStatus::running) {
        status_ = ExecuterStatus::flushing;
        insertion_.signal();
    }
}

void Executer::setEvent(const sing::ptr<Event> ev)
{
    ready_ = ev;
}

bool Executer::enqueue(const sing::iptr<Runnable> torun)
{
    if (torun == nullptr || status_ != ExecuterStatus::running) return(false);
    if (done_count_.get() + todo_count_.get() >= queue_.size()) {
        return(false); // no room
    }
    queue_[in_idx_] = torun;
    if (++in_idx_ >= queue_.size()) in_idx_ = 0;
    todo_count_.inc();
    insertion_.signal();
    return(true);
}

sing::iptr<Runnable> Executer::getRunnable(bool blocking)
{
    if (status_ != ExecuterStatus::running) return(nullptr);
    if (done_count_.get() <= 0) {
        if (!blocking) return(nullptr);
        if (ready_ != nullptr) {
            while (done_count_.get() <= 0) {
                (*ready_).wait();
            }
        } else {
            if (def_ready_ == nullptr) {
                def_ready_ = new wrapper<Event>;    
                if (def_ready_ == nullptr) return(nullptr);            
            }
            ready_ = def_ready_;
            while (done_count_.get() <= 0) {
                (*ready_).wait();
            }
            ready_ = nullptr;
        }
    }
    sing::iptr<Runnable> retvalue = queue_[out_idx_];
    queue_[out_idx_] = nullptr;     // dont' extend the life of the object through the smart pointers.
    if (++out_idx_ >= queue_.size()) out_idx_ = 0;
    done_count_.dec();
    return(retvalue);
}

bool Executer::isFlushing() const
{
    return(status_ == ExecuterStatus::flushing);
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
    while (status_ != ExecuterStatus::stopping) {

        // wait for new jobs
        insertion_.wait();

        // process or flush all the waiting jobs
        while (todo_count_.get() > 0) {
            Runnable *todo = queue_[todo_idx_];
            if (++todo_idx_ >= queue_.size()) todo_idx_ = 0;
            if (status_ == ExecuterStatus::running) {
                todo->work();
            }
            done_count_.inc();
            todo_count_.dec();
            if (ready_ != nullptr && status_ == ExecuterStatus::running) {
                (*ready_).signal();
            }
        }

        // signal that flushing is done (if we are here, all the jobs are done)
        if (status_ == ExecuterStatus::flushing) {
            status_ = ExecuterStatus::running;
            stopped_.signal();
        }
    }
    status_ = ExecuterStatus::stopped;
    stopped_.signal();
}

}   // namespace