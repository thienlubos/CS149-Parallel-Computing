#include "tasksys.h"
#include <thread>

// https://www.youtube.com/watch?v=6re5U82KwbY

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads = num_threads;
    this->threads = new std::thread[num_threads];
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    delete[] this->threads;
}

void TaskSystemParallelSpawn::threadFunc(IRunnable* runnable, int num_total_tasks, std::mutex* mutex, int* counter) {
    while (true) {
        mutex->lock();
        int task_id = *counter;
        *counter += 1;
        mutex->unlock();
        if (task_id >= num_total_tasks) {
            break;
        }
        runnable->runTask(task_id, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //  
    std::mutex *mutex = new std::mutex();
    int *counter = new int(0);

    for (int i = 0; i < this->num_threads; i++) {
        this->threads[i] = std::thread(&TaskSystemParallelSpawn::threadFunc, this, runnable, num_total_tasks, mutex, counter);
    }

    for (int i = 0; i < this->num_threads; i++) {
        this->threads[i].join();
    }

    delete mutex;
    delete counter;
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


Tasks::Tasks()
{
    this->mutex = new std::mutex(); 
    this->finished_mutex = new std::mutex();
    this->finished = new std::condition_variable();
    this->num_total_tasks = 0;
    this->num_completed_tasks = 0;
    this->num_remaining_tasks = 0;
    this->runnable = nullptr;
}

Tasks::~Tasks()
{
    delete this->mutex;
    delete this->finished_mutex;
    delete this->finished;
}

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
    this->tasks = new Tasks();
    this->num_threads = num_threads;
    this->threads = new std::thread[num_threads];
    this->done = false;

    for (int i = 0; i < this->num_threads; i++) {
        this->threads[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::threadFunc, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    this->done = true;
    for (int i = 0; i < this->num_threads; i++) {
        this->threads[i].join();
    }

    delete this->tasks;
    delete[] this->threads;
}

void TaskSystemParallelThreadPoolSpinning::threadFunc() {
    int total_tasks;
    int task_id;
    // Spinning thread
    while (!this->done) {
        // Check if there are tasks to run
        this->tasks->mutex->lock();
        total_tasks = this->tasks->num_total_tasks;
        task_id = total_tasks - this->tasks->num_remaining_tasks;
        
        if (task_id < total_tasks) {
            this->tasks->num_remaining_tasks--;
            this->tasks->mutex->unlock();

            tasks->runnable->runTask(task_id, total_tasks);
            this->tasks->mutex->lock();
            this->tasks->num_completed_tasks++;
            if (this->tasks->num_completed_tasks == total_tasks) {
                this->tasks->mutex->unlock();
                this->tasks->finished_mutex->lock();
                this->tasks->finished_mutex->unlock();
                this->tasks->finished->notify_all();
            }
            else {
                this->tasks->mutex->unlock();
            }
        } 
        else {
            this->tasks->mutex->unlock();
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lock(*(this->tasks->finished_mutex));
    this->tasks->mutex->lock();
    this->tasks->num_completed_tasks = 0;
    this->tasks->num_total_tasks = num_total_tasks;
    this->tasks->num_remaining_tasks = num_total_tasks;
    this->tasks->runnable = runnable;
    this->tasks->mutex->unlock();
    this->tasks->finished->wait(lock);
    lock.unlock();
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->tasks = new Tasks();
    this->num_threads = num_threads;
    this->threads = new std::thread[num_threads];
    this->done = false;
    this->has_tasks_mutex = new std::mutex();
    this->has_tasks = new std::condition_variable();

    for (int i = 0; i < this->num_threads; i++) {
        this->threads[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::threadFunc, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->done = true;
    this->has_tasks->notify_all();
    for (int i = 0; i < this->num_threads; i++) this->threads[i].join();

    delete this->tasks;
    delete[] this->threads;
    delete this->has_tasks_mutex;
    delete this->has_tasks;
}

void TaskSystemParallelThreadPoolSleeping::threadFunc() {
    int total_tasks;
    int task_id;
    // Spinning thread
    while (!this->done) {
        // Check if there are tasks to run
        this->tasks->mutex->lock();
        total_tasks = this->tasks->num_total_tasks;
        task_id = total_tasks - this->tasks->num_remaining_tasks;
        
        if (task_id < total_tasks) {
            this->tasks->num_remaining_tasks--;
            this->tasks->mutex->unlock();
            
            tasks->runnable->runTask(task_id, total_tasks);
            this->tasks->mutex->lock();
            this->tasks->num_completed_tasks++;
            if (this->tasks->num_completed_tasks == total_tasks) {
                this->tasks->mutex->unlock();
                this->tasks->finished_mutex->lock();
                this->tasks->finished_mutex->unlock();
                this->tasks->finished->notify_all();
            }
            else {
                this->tasks->mutex->unlock();
            }
        } 
        else {
            this->tasks->mutex->unlock();
            std::unique_lock<std::mutex> lock(*(this->has_tasks_mutex));
            this->has_tasks->wait(lock);
            lock.unlock();
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    std::unique_lock<std::mutex> lock(*(this->tasks->finished_mutex));
    this->tasks->mutex->lock();
    this->tasks->num_completed_tasks = 0;
    this->tasks->num_total_tasks = num_total_tasks;
    this->tasks->num_remaining_tasks = num_total_tasks;
    this->tasks->runnable = runnable;
    this->tasks->mutex->unlock();
    this->has_tasks->notify_all();
    this->tasks->finished->wait(lock);
    lock.unlock();
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
