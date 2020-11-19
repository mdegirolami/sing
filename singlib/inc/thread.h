#pragma once

#include <sing.h>

namespace sing {

class Runnable {
public:
    virtual ~Runnable() {}
    virtual void *get__id() const = 0;
    virtual void work() = 0;
};

class Lock final {
    void *impl = nullptr;
public:
    ~Lock();
    bool init(int32_t spincount = 100);
    void lock();
    void unlock();
    bool trylock();                     // returns true if acquires the lock
};

class Event final {
    void *impl = nullptr;
public:
    ~Event();
    bool init();
    void wait();
    void signal();
};

class Atomic final {
    volatile unsigned int atomic = 0;
public:
    void set(int32_t value);
    int32_t get() const;
    int32_t inc();
    int32_t dec();
};

class Executer final {
public:
    Executer();
    ~Executer();
    bool start(int32_t queue_len = 16, int32_t stack_size = 0);
    void stop();    // stop processing and clears all the queues.
    void flush();                       // just clears all the queues.
    void startFlushing();               // must be followed by a blocking flush to complete the task
    void setEvent(std::shared_ptr<Event> ev);               // gets signaled each time a runnable finishes

    bool enqueue(std::shared_ptr<Runnable> torun);          // returns false if the queue is full
    std::shared_ptr<Runnable> getRunnable(bool blocking = false);               // from output queue, returns null if no processed Runnable is present

    bool isFlushing() const;
    int32_t numQueued() const;          // number of queued jobs (includes the executing one)
    int32_t numDone() const;            // number of jobs in te output queue
    
    void Run();
private:
    Event insertion_;
    Event stopped_;
    std::shared_ptr<Event> def_ready_;
    std::shared_ptr<Event> ready_;
    Atomic status_;

    // queue
    std::vector<std::shared_ptr<Runnable>> queue_;
    int32_t in_idx_;
    int32_t out_idx_;
    int32_t todo_idx_;
    Atomic done_count_;
    Atomic todo_count_;
};

void runFn(void (*torun)(), int32_t stack_size = 0);
void run(std::shared_ptr<Runnable> torun, int32_t stack_size = 0);
int32_t numCores();

}   // namespace
