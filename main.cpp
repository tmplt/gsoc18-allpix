#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <array>
#include <iomanip>
#include <thread>

/*
 *  This project shall consist of:
 *  - an executable which initializes a Merseene Twister using standard seeding approaches.
 *  - several modules:
 *    - derived from the same base class
 *    - shall contain a construcor and a run() function (called multiple times)
 *    - shall have its own PRNG
 *    - return a string with its name and two numbers drawn from its PRNG
 *
 *  The framework shall be capable of the following:
 *  - execute all modules in fixed order (one full execution of all modules is called an event)
 *  - execute them n times, i.e. processing n events.
 *  - seeding all module PRNGs in each of the n events with a different number from the main PRNG
 *  - executing events in parallel, possibly using fixed number of worker threads.
 *  - joining the events after execution, retaining the order of the seeds provided by the main PRNG.
 *  - printing all return strings in the correct order.
 */

/*
 * Thoughts and questions:
 * - How should modules be initialized? Name and main prng?
 * - What defines a module? A seperate ELF dynamically loaded or just a class instance?
 */

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

    auto arg = args.cbegin();
    const int seed = argtoint(arg++);
    const int events = (argc >= 3) ?
        argtoint(arg++) : 1; /* default to a single event. */

    std::cout << "seed: " << seed << ", events: " << events << "\n";

    /* 64-bit Mersenne Twister */
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

    /*
     * TODO: the following:
     * - execute the events in parallel, possibly using a fixed number of worker threads.
     * - joining the events after execution, retaining the order of the seeds provided by the main PRNG
     *   (build_event() must be called in the same order).
     * - printing all return strings in the correct order.
     *
     * No matter the number of threads, the output has to be the same.
     */

    std::vector<std::string> results(events);
    for (auto &result : results) {
        std::stringstream ss;

        for (auto &module : build_event())
            ss << module.run() << "\n";

        result = ss.str();
    }

    for (const auto &result : results) {
        std::cout << result << "\n";
    }
}
