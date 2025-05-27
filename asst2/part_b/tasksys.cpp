#include <thread>
#include <iostream>
#include "tasksys.h"

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) : num_threads(num_threads) {}
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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    threads.reserve(num_threads);
    // Book keeper thread
    threads.emplace_back(
        [this]() {
            // For each entry in waiting_launches, constantly check its num_dependency, if it's 0, delete it from waiting_launches and 
            // num_dependency, enqueue all of its tasks to the task queue.
            while (true) {
                std::unique_lock<std::mutex> lock(wait_queue_mutex);
                condition.wait(lock, [this]() {return sync_flag || !running;});
                if (!running && launch_counter==0) {
                    return;
                }
                for (auto it = waiting_launches.begin(); it != waiting_launches.end();) {
                    const TaskID& launch_id = it->first;
                    const BulkLaunch& launch = it->second;
                    if (num_dependency[launch_id] == 0) {
                        task_counters[launch_id] = 0;
                        for (int i = 0; i < launch.num_total_tasks; i++) {
                            task_queue.push(
                                {launch_id, 
                                [=]() { launch.runnable->runTask(i, launch.num_total_tasks); }}
                            );
                            task_counters[launch_id]++;
                        }
                        condition.notify_all();  // wake up the worker thread
                        it = waiting_launches.erase(it);  // Safely erase and update iterator
                        num_dependency.erase(launch_id);
                    } else {
                        it++;  // Move to next element only if not erased
                    }
                }
                // If no more ready launch in the waiting queue, go to sleep, it will be awaken when a launch is done.
                if (launch_counter==0){
                    all_launches_done = true;
                    condition.notify_all();
                    return;
                }

            }
        }
    );

    for (int i = 1; i < num_threads; i++) {
        threads.emplace_back(
            [this]() {
                while(true) {
                    std::pair<TaskID, std::function<void()>> pair;
                    std::function<void()> task;
                    TaskID launch_id; 
                    {
                        std::unique_lock<std::mutex> lock (queue_mutex);
                        condition.wait(lock, [this]() {return (sync_flag && !task_queue.empty()) || !running;});
                        if (!running && task_queue.empty()) {
                            return;
                        }
                        pair = std::move(task_queue.front());
                        task_queue.pop();
                    }
                    launch_id = pair.first;
                    task = pair.second;
                    task();
                    {
                        std::unique_lock<std::mutex> lock (queue_mutex);
                        task_counters[launch_id] --;
                        // A launch is done, decrease the dependency count for all of its dependents, notify the book keeper thread so that it can enqueue new tasks
                        if (task_counters[launch_id] == 0) {

                            launch_counter--;
                            for(auto& dependent_launch_id : dependents[launch_id]) {
                                num_dependency[dependent_launch_id] --;
                            }
                            dependents.erase(launch_id);
                            condition.notify_all();  // Awake the book keeper thread
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
    {
        std::unique_lock<std::mutex> lock (queue_mutex);
        running = false;
        condition.notify_all();
        // Clear containers to ensure no stale data
    }
    condition.notify_all(); // Second notification to catch stragglers
    
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

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //
    const TaskID launch_id = next_launch_id;
    next_launch_id++;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        // Store the current bulk launch in waiting_launches
        waiting_launches[launch_id] = {runnable, num_total_tasks, deps};
        launch_counter++;
        // Initialize dependency count
        num_dependency[launch_id] = deps.size();

        // QUESTION Seems like, for every previous launch in deps, add the current launch as a dependent
        for (TaskID dep_id : deps) {
            dependents[dep_id].insert(launch_id);
        }
        condition.notify_all(); // Wake up the book keeper thread
    }
    return launch_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    std::unique_lock<std::mutex> lock(queue_mutex);
    sync_flag = true;
    condition.notify_all();
    condition.wait(lock, [this]() { return all_launches_done; });
    
    return;
}
