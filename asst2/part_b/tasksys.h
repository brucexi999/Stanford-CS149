#ifndef _TASKSYS_H
#define _TASKSYS_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <set>
#include "itasksys.h"

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();

    private:
        // Vector to store worker threads
        std::vector<std::thread> threads;
        // Queue of tasks
        std::queue<std::pair<TaskID, std::function<void()>>> task_queue;
        // Mutex to synchronize access to shared data
        std::mutex queue_mutex;
        std::mutex wait_queue_mutex;
        // Condition variable to signal changes in the state of the task queue
        std::condition_variable condition;
        // Signals threads to exit
        // std::atomic<bool> running{true};
        bool running = true;

        struct BulkLaunch
        {
            IRunnable* runnable;
            int num_total_tasks;
            std::vector<TaskID> deps;
        };

        // TaskID is actually the ID for each bulk task launch, not the IDs for individual tasks within 1 launch
        TaskID next_launch_id = 0;  // Start the ID at 1, it is said 0 can cause problem
        std::unordered_map<TaskID, BulkLaunch> waiting_launches; // The launches that are not ready to run due to dependencies
        std::unordered_map<TaskID, std::set<TaskID>> dependents;  // The dependencies (a set of TaskIDs of previous launches) of each launch (indexed by its TaskID), QUESTION why the set data structure? Why not just store the vector?
        std::unordered_map<TaskID, int> num_dependency;  // The remaining dependency count for a launch
        std::unordered_map<TaskID, int> task_counters;
        unsigned int launch_counter = 0;
        bool sync_flag = false;
        bool all_launches_done = false;
};

#endif
