#include "thread_test.h"
#include "thread.h"
#include "sys.h"
#include "sio.h"

class Add final : public sing::Runnable {
public:
    Add();
    virtual void *get__id() const override { return(&id__); };
    void init(std::shared_ptr<std::vector<float>> v1, std::shared_ptr<std::vector<float>> v2, int32_t start, int32_t stop);
    virtual void work() override;

    static char id__;

private:
    std::shared_ptr<std::vector<float>> v1_;
    std::shared_ptr<std::vector<float>> v2_;
    int32_t start_;
    int32_t stop_;
};

static void DoAll(std::shared_ptr<std::vector<float>> v1, std::shared_ptr<std::vector<float>> v2, int32_t count);
static void increments();

static sing::Event done;
static sing::Lock lock;
static sing::Atomic atomic;
static int32_t counter = 0;

char Add::id__;

bool thread_test()
{
    const std::shared_ptr<int32_t> ttt;

    if (ttt == nullptr) {
        if (nullptr != ttt) {
        }
    }
    const int32_t cores = sing::numCores();
    if (cores < 2) {
        return (false);
    }

    bool tryok = false;                 // to check trylock coverage
    bool tryko = false;
    done.init();
    lock.init();
    sing::runFn(increments);
    for(int32_t ii = 0; ii < 10000; ++ii) {
        if (lock.trylock()) {
            tryok = true;
        } else {
            lock.lock();
            tryko = true;
        }
        ++counter;
        for(int32_t jj = 0; jj < 1000; ++jj) {              // to ease lock conflicts
        }
        lock.unlock();
        atomic.inc();
    }
    done.wait();
    if (counter != 20000 || atomic.get() != 20000 || !tryok || !tryko) {
        return (false);
    }

    std::shared_ptr<std::vector<float>> v1 = std::make_shared<std::vector<float>>();
    std::shared_ptr<std::vector<float>> v2 = std::make_shared<std::vector<float>>();
    for(int32_t ii = 0; ii < 1000000; ++ii) {
        (*v1).push_back(5.0f);
        (*v2).push_back(3.0f);
    }

    int64_t start = sing::clock();
    for(int32_t ii = 0; ii < 1000000; ++ii) {
        (*v1)[ii] += (*v2)[ii];
    }
    const int64_t single_thread = sing::clocksDiff(start, sing::clock());

    start = sing::clock();
    DoAll(v1, v2, cores);
    const int64_t multiple_threads = sing::clocksDiff(start, sing::clock());

    sing::print(sing::s_format("%s%lld%s%lld", "\nsingle = ", single_thread, "\ndouble = ", multiple_threads).c_str());
    sing::print(sing::s_format("%s%d%s", "\ncores = ", cores, "\npress any key.").c_str());
    sing::kbdGet();

    std::shared_ptr<Add> adder = std::make_shared<Add>();
    (*adder).init(v1, v2, 0, 10);
    done.wait();    // to reset
    sing::run(adder);
    done.wait();
    if ((*v1)[0] != 14.0f) {
        return (false);
    }
    return (true);
}

Add::Add()
{
    start_ = 0;
    stop_ = 0;
}

void Add::init(std::shared_ptr<std::vector<float>> v1, std::shared_ptr<std::vector<float>> v2, int32_t start, int32_t stop)
{
    this->v1_ = v1;
    this->v2_ = v2;
    this->start_ = start;
    this->stop_ = stop;
}

void Add::work()
{
    for(int32_t ii = start_, ii__top = stop_; ii < ii__top; ++ii) {
        (*v1_)[ii] += (*v2_)[ii];
    }
    done.signal();
}

static void DoAll(std::shared_ptr<std::vector<float>> v1, std::shared_ptr<std::vector<float>> v2, int32_t count)
{
    std::vector<sing::Executer> executers;
    executers.resize(count);
    const int32_t len = (*v2).size() / count;
    int64_t idx = -1;
    for(auto &ex : executers) {
        ++idx;
        std::shared_ptr<Add> adder = std::make_shared<Add>();
        const int32_t start = (int32_t)idx * len;
        if ((int32_t)idx == count - 1) {
            (*adder).init(v1, v2, start, start + len);
        } else {
            (*adder).init(v1, v2, start, (*v2).size());
        }
        ex.enqueue(adder);
        ex.start();
    }
    for(auto &ex : executers) {
        ex.getRunnable(true);
    }
}

static void increments()
{
    for(int32_t ii = 0; ii < 10000; ++ii) {
        lock.lock();
        for(int32_t jj = 0; jj < 1000; ++jj) {              // to ease lock conflicts
        }
        ++counter;
        lock.unlock();
        atomic.inc();
    }
    done.signal();
}
