/*
 * From <https://github.com/progschj/ThreadPool>.
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    /*
     * For each wanted worker, create a thread that waits for
     * tasks to be added to the queue. When the task has been run,
     * wait for another task.
     */
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        /*
                         * Block thread until notified via condition variable.
                         * When unblocked, check if pool should stop running tasks
                         * or if there are tasks available. If this isn't the case,
                         * wait for next notification.
                         */
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });

                        /*
                         * Conditions for stopping the pool.
                         */
                        if(this->stop && this->tasks.empty())
                            return;

                        /* A task is availabel, pop it from the queue ... */
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    /* ... and run it */
                    task();
                }
            }
        );
}

// add new work item to the pool
/* This template allows a lambda of any type to be enqueued. */
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    /* Package the lambda with its arguments so that it may be called asynchronously. */
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        /* Emplace the task in the queue, from which workers will pop from. */
        tasks.emplace([task](){ (*task)(); });
    }

    /* Notify a waiting worker that a task is available. */
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        /* Signal a stop; no more tasks can be enqueued. */
        stop = true;
    }

    /* Notify all waiting workers to finish the queue of tasks. */
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif

