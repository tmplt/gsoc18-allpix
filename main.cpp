#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <iterator>
#include <algorithm>

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
    module_base(std::string name)
        : name(name) {}

    module_base(std::string name, int seed)
        : module_base(name)
    {
        prng.seed(seed);
    }

    std::string run()
    {
        std::stringstream ret;
        ret << name << "\t" << prng() << prng();
        return ret.str();
    }

protected:
    std::mt19937 prng;
    std::string name;
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <seed> [events] [-- seed..]\n";
        return EXIT_FAILURE;
    }

    const std::vector<std::string> args(argv + 1, argv + argc);

    /*
     * This can be improved by using a C++17 std::optional;
     * it's not very nice to std::exit, especially in a lambda.
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

    /* Default to a single event. */
    const int events = (argc >= 3 && *arg != "--") ? argtoint(arg++) : 1;

    std::vector<int> seeds;
    if (arg != args.cend() && *arg == "--") {
        for (++arg; arg != args.cend(); ++arg)
            seeds.push_back(argtoint(arg));
    }

    std::cout << "seed: " << seed << ", events: " << events << "\n";

    for (auto &seed : seeds)
        std::cout << seed << (seed != seeds.back() ? " " : "\n");

    /* 32-bit Mersenne Twister */
    std::mt19937 prng;
    prng.seed(1);
}
