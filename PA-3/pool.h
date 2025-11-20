#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <unordered_map>

class Task {
public:
    Task();

    virtual ~Task();

    virtual void Run() = 0;  // implemented by subclass, virtual function!!
    bool is_running() const { return running; } // returns if Task is running

    std::string name;
    bool running = false;
};

class ThreadPool {
public:
    explicit ThreadPool(int num_threads); // initialized with number of threads

    ~ThreadPool();

    // Submit a task with a particular name.
    void SubmitTask(const std::string &name, Task *task); // takes a task with name
    void WaitForTask(const std::string &name);
    void remove_task(Task *t);

    // Stop all threads. All tasks must have been waited for before calling this.
    // You may assume that SubmitTask() is not caled after this is called.
    void Stop();

    void run_thread(); // 

    int num_tasks_unserviced = 0;

    // struct to link status of task to task itself
    struct TaskStatus {
        bool finished = false;
        std::condition_variable cv;
        Task* task = nullptr;
    };

    // used by WaitForTask to map name to status of task
    std::unordered_map<std::string, TaskStatus> task_status;
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::vector<std::thread *> threads; // holds all threads
    std::vector<Task *> queue; // holds all tasks that haven't been executed yet
    volatile bool done = false; // set to true once all tasks done (queue empty)
};
