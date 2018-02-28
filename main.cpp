#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <array>
#include <iomanip>
#include <thread>

class module_base {
public:
    const std::string run()
    {
        std::stringstream ret;

        /* Bad practice, but okay for this small example. */
        constexpr int hex_len = 17;

        ret << name << " " << std::hex << std::setfill(' ') << std::setw(hex_len) << std::right
            << prng() << " " << prng();

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

    std::vector<std::string> results(events);
    std::vector<std::thread> threads;

    for (auto result = results.begin(); result != results.end(); ++result) {
        /* Build event outside thread to ensure same output. */
        auto event = build_event();

        /*
         * Start the thread, calling .run() on all modules in the given event,
         * and store its result in the given string reference.
         */
        threads.emplace_back([](auto event, auto result) {
            std::stringstream ss;
            for (auto &module : event)
                ss << module.run() <<"\n";

            *result = ss.str();
        }, event, result);
    }

    /* Join all threads ... */
    for (auto &t : threads)
        t.join();

    /* ... and print the resulting output. */
    for (const auto &result : results)
        std::cout << result << "\n";
}
