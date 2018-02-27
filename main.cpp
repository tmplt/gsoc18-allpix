#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <array>
#include <iomanip>
#include <thread>

#include <mutex>
#include <memory>
#include <future>
#include <queue>
#include <condition_variable>
#include <functional>

class module_base {
public:
    const std::string run()
    {
        std::stringstream ret;
        // TODO: right-align this?
        ret << name << " " << std::hex << std::setfill(' ') << std::setw(3)
            << prng() << "  " << prng();
        return ret.str();
    }

protected:
    module_base(std::string name)
        : name(name) {}

    module_base(std::string name, int seed)
        : module_base(name)
    {
        prng.seed(seed);
    }

    std::mt19937_64 prng;
    const std::string name;
};

/* Dummy classes, all of them modules. */

class A : public module_base {
public:
    A(std::string &&name, int seed)
        : module_base(std::forward<decltype(name)>(name), seed) {}
};

class B : public module_base {
public:
    B(std::string &&name, int seed)
        : module_base(std::forward<decltype(name)>(name), seed) {}
};

class C : public module_base {
public:
    C(std::string &&name, int seed)
        : module_base(std::forward<decltype(name)>(name), seed) {}
};

class D : public module_base {
public:
    D(std::string &&name, int seed)
        : module_base(std::forward<decltype(name)>(name), seed) {}
};

/* A thread pool, only n threads (excluding main thread) run concurrently. */

class thread_pool {
public:
    thread_pool(size_t size)
        : workers(size)
    {
        for (size_t i = 0; i < size; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->mutex);

                        this->cv.wait(lock, [this] { return !this->tasks.empty(); });
                        if (this->tasks.empty()) return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    template<typename F, typename... Args>
    auto push(F &&f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        /*
         * If pool isn't full, execute function and then remove it from queue
         * when done.
         *
         * If pool is full, block until it isn't.
         * std::condition_variable no notify when a slot is free? Which slot?
         * How do we know that the function is done?
         */
        /* pool.emplace_back(std::forward<F>(f), std::forward<Args>(args)...); */

        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(mutex);
            tasks.emplace([task]() { (*task)(); });
        }

        cv.notify_one();
        return res;
    }

    void join()
    {
        cv.notify_all();
        for (auto &w: workers)
            w.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex mutex;
    std::condition_variable cv;
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <seed> [events]\n";
        return EXIT_FAILURE;
    }

    const std::vector<std::string> args(argv + 1, argv + argc);

    /*
     * This can be improved by using a C++17 std::optional and checking it externally;
     * it's not very nice to std::exit(), especially in a lambda.
     */
    const auto argtoint = [&args](auto it) -> int {
        try {
            return std::stoi(*it);
        } catch (...) { /* std::invalid_argument or std::out_of_range */
            std::cerr << "'" << *it << "' is not an integer.\n";
            std::exit(EXIT_FAILURE);
        }
    };

    /* Parse program arguments. */
    auto arg = args.cbegin();
    const int seed = argtoint(arg++);
    const int events = (argc >= 3) ?
        argtoint(arg++) : 1; /* default to a single event. */
    const int workers = (argc >= 4) ?
        argtoint(arg++) : std::thread::hardware_concurrency(); /* default to number of CPU cores. */

    std::cout << "seed: " << seed
              << ", events: " << events
              << ", workers: " << workers
              << "\n";

    /* Main 64-bit Mersenne Twister. Seeds all created modules. */
    std::mt19937_64 prng;
    prng.seed(seed);

    /* Build an event of four sequential modules using the main PRNG. */
    const auto build_event = [&prng]() -> std::array<module_base, 4> {
        return {{
            A("module1", prng()),
            B("module2", prng()),
            C("module3", prng()),
            D("module4", prng()),
        }};
    };

    /* TODO: execute the events in parallel using a fixed number of worker threads. */

    std::vector<std::string> results(events);
    thread_pool pool(workers);

    for (auto result = results.begin(); result != results.end(); ++result) {
        /* Build event outside thread to ensure same output. */
        auto event = build_event();

        /*
         * Start the thread, calling .run() on all modules in the given event,
         * and store its result in the given string reference.
         */
        pool.push([](auto event, auto result) {
            std::stringstream ss;
            for (auto &module : event)
                ss << module.run() <<"\n";

            *result = ss.str();
        }, event, result);
    }

    /* Join all threads in the pool ... */
    pool.join();

    /* ... and print the resulting output. */
    for (const auto &result : results)
        std::cout << result << "\n";
}
