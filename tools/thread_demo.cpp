#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace {

    constexpr int kIters = 100000;   // 每个线程加多少次
    constexpr int kRounds = 20;       // 每版跑多少轮（一轮 = 起两个线程）

    // ── race 版与 mutex 版共用这个普通变量 ──
    long long g_counter = 0;
    std::mutex g_m;

    // ── atomic 版专用（类型不同，必须分开）──
    std::atomic<long long> g_atomic{ 0 };

    void addNoSync() {
        for (int i = 0; i < kIters; ++i) {
            ++g_counter;                       // 读-改-写，无同步
        }
    }

    void addWithMutex() {
        for (int i = 0; i < kIters; ++i) {
            std::lock_guard<std::mutex> guard(g_m);   // 构造加锁
            ++g_counter;                              // 临界区
        }                                             // 析构解锁
    }

    void addAtomic() {
        for (int i = 0; i < kIters; ++i) {
            ++g_atomic;                        // 原子读-改-写（一条 RMW 指令）
        }
    }

    struct Result {
        long long value;    // 观测到的计数
        long long micros;   // 本轮耗时（微秒）
    };

    Result runRace() {
        g_counter = 0;
        const auto start = std::chrono::steady_clock::now();
        std::thread a(addNoSync);
        std::thread b(addNoSync);
        a.join();
        b.join();
        const auto finish = std::chrono::steady_clock::now();
        const long long us =
            std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
        return Result{ g_counter, us };
    }

    Result runMutex() {
        g_counter = 0;
        const auto start = std::chrono::steady_clock::now();
        std::thread a(addWithMutex);
        std::thread b(addWithMutex);
        a.join();
        b.join();
        const auto finish = std::chrono::steady_clock::now();
        const long long us =
            std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
        return Result{ g_counter, us };
    }

    Result runAtomic() {
        g_atomic.store(0);
        const auto start = std::chrono::steady_clock::now();
        std::thread a(addAtomic);
        std::thread b(addAtomic);
        a.join();
        b.join();
        const auto finish = std::chrono::steady_clock::now();
        const long long us =
            std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
        return Result{ g_atomic.load(), us };
    }

    // 跑 kRounds 轮，统计失败次数 / 最小值 / 最大值 / 平均耗时
    template <typename Fn>
    void report(const char* name, Fn run) {
        const long long expected = 2LL * kIters;

        long long fails = 0;
        long long minVal = 0;
        long long maxVal = 0;
        long long totalUs = 0;

        for (int r = 0; r < kRounds; ++r) {
            const Result res = run();
            if (res.value != expected) {
                ++fails;
            }
            if (r == 0) {
                minVal = res.value;
                maxVal = res.value;
            }
            else {
                if (res.value < minVal) { minVal = res.value; }
                if (res.value > maxVal) { maxVal = res.value; }
            }
            totalUs += res.micros;
        }

        std::cout << name
            << " | FAIL " << fails << "/" << kRounds
            << " | min=" << minVal
            << " | max=" << maxVal
            << " | avg=" << (totalUs / kRounds) << " us\n";
    }

}  // namespace

int main() {
    std::cout << "expected = " << (2LL * kIters)
        << " | rounds = " << kRounds << "\n";

    report("race  ", runRace);
    report("mutex ", runMutex);
    report("atomic", runAtomic);

    return 0;
}