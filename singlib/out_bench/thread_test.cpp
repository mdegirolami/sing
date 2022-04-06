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

static int64_t DoAll(std::shared_ptr<std::vector<float>> v1, std::shared_ptr<std::vector<float>> v2, int32_t count);
static void increments();

static sing::Event done;
static sing::Lock lock;
static sing::Atomic atomic;
static int32_t counter = 0;

char Add::id__;

bool thread_test()
{
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
        if (ii % 100 == 0) {
            sing::wait(1);              // to ease conflicts
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
    for(int32_t ii = 0; ii < 10000000; ++ii) {
        (*v1).push_back(5.0f);
        (*v2).push_back(3.0f);
    }

    const int64_t start = sing::clock();
    for(int32_t ii = 0, ii__top = (*v1).size(); ii < ii__top; ++ii) {
        (*v1)[ii] += (*v2)[ii];
    }
    const int64_t single_thread = sing::clocksDiff(start, sing::clock());
    const int64_t multiple_threads = DoAll(v1, v2, 2);

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
    const std::shared_ptr<std::vector<float>> src1 = v1_;
    const std::shared_ptr<std::vector<float>> src2 = v2_;
    if (src1 != nullptr && src2 != nullptr) {
        for(int32_t ii = start_, ii__top = stop_; ii < ii__top; ++ii) {
            (*src1)[ii] += (*src2)[ii];
        }
    }
    done.signal();
}

static int64_t DoAll(std::shared_ptr<std::vector<float>> v1, std::shared_ptr<std::vector<float>> v2, int32_t count)
{
    if (v2 == nullptr || count == 0) {
        return (0);
    }
    std::vector<std::shared_ptr<sing::Executer>> executers;
    executers.resize(count);
    const int32_t len = (*v2).size() / count;
    for(auto &ptr : executers) {
        std::shared_ptr<sing::Executer> ex = std::make_shared<sing::Executer>();
        (*ex).start();
        ptr = ex;
    }
    sing::wait(1);                      // be sure the threads are running !!

    const int64_t start = sing::clock();

    int64_t idx = -1;
    for(auto &ex : executers) {
        ++idx;
        std::shared_ptr<Add> adder = std::make_shared<Add>();
        const int32_t start_idx = (int32_t)idx * len;
        if ((int32_t)idx != count - 1) {
            (*adder).init(v1, v2, start_idx, start_idx + len);
        } else {
            (*adder).init(v1, v2, start_idx, (*v2).size());
        }
        const std::shared_ptr<sing::Executer> exp = ex;
        if (exp != nullptr) {
            (*exp).enqueue(adder);
        }
    }
    for(auto &ex : executers) {
        const std::shared_ptr<sing::Executer> exp = ex;
        if (exp != nullptr) {
            (*exp).getRunnable(true);
        }
    }
    return (sing::clocksDiff(start, sing::clock()));
}

static void increments()
{
    for(int32_t ii = 0; ii < 10000; ++ii) {
        lock.lock();
        if (ii % 100 == 0) {
            sing::wait(1);              // to ease conflicts
        }
        ++counter;
        lock.unlock();
        atomic.inc();
    }
    done.signal();
}
