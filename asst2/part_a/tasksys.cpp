#include <thread>
#include <iostream>
#include "tasksys.h"

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads): num_threads(num_threads){}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads)
    : ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }

    // Lambda function for each thread
    auto run_tasks = [=](int thread_id) {
        int tasks_per_thread = (num_total_tasks + num_threads -1) / num_threads;
        int start = thread_id * tasks_per_thread;
        int end = std::min(start + tasks_per_thread, num_total_tasks);
        for (int i = start; i < end; i++) {
            runnable->runTask(i, num_total_tasks);
        }
    };

    // Main thread will be used as a worker hence num_threads - 1
    std::thread* threads = new std::thread[num_threads - 1];
    for (int i = 0; i < num_threads - 1; i++) {
        threads[i] = std::thread(run_tasks, i);
    }

    run_tasks(num_threads-1);

    for (int i = 0; i < num_threads - 1; i++) {
        threads[i].join();
    }
    delete[] threads;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(
            [this]() {
                while(true) {
                    std::function <void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        // printf("Mutex acquired\n");
                        if (!task_queue.empty() && running) {
                            task = std::move(task_queue.front());
                            task_queue.pop();
                        }
                        else if (task_queue.empty() && !running) {
                            return;
                        }
                        else {
                            lock.unlock();
                            std::this_thread::yield();
                            continue;
                        }
                    }
                    task();
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        task_counter --;
                    }
                }
            }
        );
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    running = false;
    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        } 
    }
    
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        for (int i=0; i<num_total_tasks; i++) {
            task_queue.push(
                [=](){
                    runnable->runTask(i, num_total_tasks);
                }
            );
            task_counter ++;
        }
    }
    // Wait for all tasks to complete
    while (true) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (task_counter != 0) {
                lock.unlock();
                std::this_thread::yield();
                continue;
            }
            else {
                return;
            }
        }
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads), running(true) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    threads.reserve(num_threads);

    for (int i = 0; i < num_threads; i++) {
        // In C++, the vector emplace_back() is a built-in method used to insert an element at the end of the vector by constructing it in-place. 
        // It means that the element is created directly in the memory allocated to the vector avoiding unnecessary copies or moves.
        // Each thread in the vector runs continuously to wait for tasks enqueued by the run function. 
        threads.emplace_back(
            [this]() {  // Lambda function for each thread, Capture Clause: [this] captures the this pointer, allowing the lambda to access the TaskSystemParallelSpawn object’s members
                while(true) {  // Keep the thread running 
                    /*
                    std::function<void()> is a type from the <functional> header that represents a callable object (e.g., function, lambda, functor) with no parameters (()) and no return value (void).
                    task is a variable that holds a single task dequeued from task_queue.
                    In this context, task will store a lambda that executes a range of runnable->runTask calls (enqueued by run).
                    */
                    std::function <void()> task;
                    {  // Opens a scope block to limit the lifetime of lock. Ensures queue_mutex is unlocked when the block ends before executing task(). 
                       // This prevents holding the mutex during task execution, which could block other threads or run from enqueuing tasks.
                        // When a thread executes the below line of code, it acquires queue_mutex, locks it, preventing other threads from accessing task_queue, avoiding race condition.
                        // The mutex lock is released automatically when the unique_lock object is destroyed (end of the { ... } block), preventing deadlocks from forgotten unlocks.
                        std::unique_lock<std::mutex> lock (queue_mutex);
                        // wait puts the thread in sleep (blocking subsequence instructions) until the condition defined by the lambda is met. The lock on mutex is temporarily released while it's waiting. 
                        // This allows the run function to enqueue tasks. Otherwise, this thread will just grab the mutex and go to sleep with others can't get access to the task queue.
                        // 1. Acquire the mutex, if not, wait and block subsequent code
                        // 2. Once mutex is acquired, check the condition, if true, continue the subsequent code, if false, release the mutex and go to sleep
                        // 3. When notified, it wakes up and tries to get the mutex again, back to step 1
                        condition.wait(lock, [this] () {  // The awaken condition must be a function, so a lambda can do the job
                            return !task_queue.empty() || !running;
                        });
                        if (!running && task_queue.empty()) {  // If no tasks left and the destructor is called, exit
                            return;  // Exit thread by exiting the infinite loop
                        }
                        // task_queue.front(): Gets a reference to the first task (a std::function<void()> lambda) in task_queue.
                        // std::move: Moves the task to the task variable, avoiding a copy for efficiency.
                        // task_queue.pop(): Removes the task from the queue.
                        task = std::move(task_queue.front());
                        task_queue.pop();
                    }  // Ends the scope, destroying lock and unlocking queue_mutex. Holding the mutex during task() would cause contention, slowing down the system.
                    task();
                    {
                        std::unique_lock<std::mutex> lock (queue_mutex);
                        task_counter --;
                        if (task_queue.empty()) {
                            condition.notify_all();
                        }
                    }
                }
            }

        );
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    
    running = false;
    condition.notify_all();
    
    for (auto& thread: threads) {  // auto& thread: thread is a reference (&) to each std::thread object in threads, allowing modification (e.g., calling join()).
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }
    {
        // Lock the task queue, enqueue all the tasks, then wake all workers.
        std::unique_lock<std::mutex> lock (queue_mutex);
        for (int i = 0; i < num_total_tasks; i++) {
            task_queue.push([=](){
                runnable->runTask(i, num_total_tasks);
            });
            task_counter ++;
        }
        condition.notify_all();
    }
    std::unique_lock<std::mutex> lock(queue_mutex);
    condition.wait(lock, [this]() { return task_counter == 0; });
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
