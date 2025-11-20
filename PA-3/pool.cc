#include "pool.h"
#include <mutex>
#include <iostream>

// mutex to serialize console output so lines don't get interleaved
static std::mutex print_mtx;

Task::Task() = default;
Task::~Task() = default;

ThreadPool::ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(new std::thread(&ThreadPool::run_thread, this));
    }
}

ThreadPool::~ThreadPool() {
    for (std::thread *t: threads) {
        delete t;
    }
    threads.clear();

    for (Task *q: queue) {
        delete q;
    }
    queue.clear();
}

void ThreadPool::WaitForTask(const std::string &name) {
    // unique_lock prevents multiple threads from accessing our task_status map at once
    std::unique_lock<std::mutex> lock(mtx);

    // find task with name name in task_status
    auto it = task_status.find(name);
    if (it == task_status.end()) {
        throw std::runtime_error("unknown task name");
    }

    TaskStatus &status = it->second;

    // wait until task is finished
    while (!status.finished) {
        status.cv.wait(lock);
    }

    task_status.erase(it);
}

void ThreadPool::SubmitTask(const std::string &name, Task *task) {
    // if Stop() called, do not allow tasks to be submitted
    if (done) {
        {
            std::lock_guard<std::mutex> pl(print_mtx);
            std::cout << "Cannot added task to queue" << std::endl;
        }
        return;
    }

    mtx.lock(); // prevents multiple threads from accessing queue at once

    queue.push_back(task); // add task to queue
    
    task->name = name;
    task_status[name].task = task;
    task_status[name].finished = false;

    mtx.unlock(); // unlock mutex to allow for other thread control after queue access

    {
        std::lock_guard<std::mutex> pl(print_mtx);
        std::cout << "Added task " << name << std::endl;
    }

    cv.notify_one();
}

void ThreadPool::run_thread() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        // wait until there's work or we're done
        cv.wait(lock, [this] { return done || !queue.empty(); });

        // done is true and queue is empty, all threads are done/released
        if (done && queue.empty()) {
            {
                std::lock_guard<std::mutex> pl(print_mtx);
                std::cout << "Stopping thread" << std::endl;
            }
            break;
        }

        // get task from queue and remove it
        Task *t = queue.front();
        queue.erase(queue.begin());
        lock.unlock();

        {
            std::lock_guard<std::mutex> pl(print_mtx);
            std::cout << "Started task " << t->name << std::endl;
        }
        t->Run();
        {
            std::lock_guard<std::mutex> pl(print_mtx);
            std::cout << "Finished task " << t->name << std::endl;
        }

        // second lock to prevent multi-thread access to task_status
        std::unique_lock<std::mutex> lock2(mtx);
        task_status[t->name].finished = true;
        task_status[t->name].cv.notify_one();
        lock2.unlock();

        delete t;
    }
}

// Remove Task t from queue if it's there
void ThreadPool::remove_task(Task *t) {
    mtx.lock();
    for (auto it = queue.begin(); it != queue.end();) {
        if (*it == t) {
            queue.erase(it);
            mtx.unlock();
            return;
        }
        ++it;
    }
    mtx.unlock();
}

void ThreadPool::Stop() {
    {
        std::lock_guard<std::mutex> pl(print_mtx);
        std::cout << "Called Stop()" << std::endl;
    }

    mtx.lock();

    // enable done to signal Stop() called
    done = true;

    // awakens all waiting threads
    cv.notify_all();
    mtx.unlock();

    // join all threads
    for (std::thread* t : threads) {
        if (t->joinable()) {
            t->join();
        }
        delete t;
    }
    threads.clear();
}
