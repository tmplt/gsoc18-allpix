Google Summer of Code 2018 — Allpix² evaluation task
---

```sh
$ ./cernmt <seed> [events]
```

Example output:
```sh
$ ./cernmt 42 4
seed: 42, events: 4
module1  3a55f78b4b19ab26 352253f18d904a4b
module2  ce16a82ed4764595 af6baff84154e315
module3  34c32694b8d963d2 911ce653108f0ffb
module4  808cbea189ac9b7e 60f9ba09e20bbebf

module1  74e2d7ec15ac713f 575dbb923696b34a
module2  e2753b827cb59f1f 29f21a4a535367ee
module3  77a25c610558edd8 4ec1ae84beb62274
module4  8a25d4f6651e2505 b2b3b224d14dd7e2

module1  145ca54719376cf3 134c5df159d2d152
module2  aecec669b5b58719 a83f1bdf6e90ca82
module3   4298366e122621e 849d14793cf3415b
module4  6ca4ed3c09260d73 db7b15f9e99881cf

module1  83ec41ab46b1a5eb 3a1f5248697616b
module2  aa482dd429fcd642 85f07604a8f241cf
module3  5a57b3e61a82eeba a4486fd49d7fd54a
module4  6204d6d7705c35b2 5dab819a4e2d3bf7
```

This project is an implementation of [the evaluation task for the Allpix² project](http://hepsoftwarefoundation.org/gsoc/2018/proposal_AllpixSquaredEventMultithreading.html),
under [the CERN-HSF organization](https://summerofcode.withgoogle.com/organizations/5377828787322880/).

The task is to prepare a short framework, written in C++11/14, which contains a pseudo-RNG source of Mersenne Twister type.
Several modules derived from the same base class shall be written and use this source to seed their internal sources.
A module shall expose a `run()` member function that returns a string containing its name and two numbers drawn from its internal PRNG source.

This framework is capable of
* executing the defined modules in a sequential order (one full execution is called an "event");
* executing *n* events (via positional argument `[events]`);
* seeding all module PRNG sources from the main PRNG (seed can be passed via the positional argument `<seed>`);
* executing these events in parallel (via one thread per event) and
* retaining program output, independent on how many events are processed concurrently.

Order of execution
---

When the program starts
1. Program arguments are parsed. If a non-integer is passed, the program exits with `EXIT_FAILURE`.
2. The main Mersenne Twister is created and seeded with the given seed.
3. For each event to execute, a thread is spawned and `.run()` is called on every module in a unique event. The string combination is then saved to a vector.
4. After joining all threads, the vector of results is then printed.

The `thread_pool` branch creates a pool of worker threads of fixed size (defaults to number of CPU cores, can be specified via positional argument `[workers]`).
Instead of running all threads concurrently, all event executions are added to a queue, from which the workers pop from.
In this branch, the result vector now contains `std::future<std::string>`s instead.
The [progschj/ThreadPool](https://github.com/progschj/ThreadPool) library was used for the implementation of the thread pool.

Possible improvements
---

`cernmt` take two positional arguments, one of which is optional.
It is probably a better idea to use flags with arguments for optional arguments.

If a non-integer is passed to the program, `std::exit()` is called from within a lambda.
It would be much better if any program exits occur via `return`s instead.

Building instructions
---

```sh
$ mkdir build && cd build
$ cmake ..
$ make
```
