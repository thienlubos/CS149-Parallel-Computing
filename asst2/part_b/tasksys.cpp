#include "tasksys.h"
#include <stdio.h>
#include <iostream> 
#include <fstream>

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
 * Tasks Implementation
 * ================================================================
 */

Task::Task(IRunnable* runnable, int num_total_tasks, const std::vector<TaskID> &deps)
{   
    this->num_tasks = num_total_tasks;
    this->num_done = 0;
    this->num_left = num_total_tasks;
    this->task_id = -1;     // detached task
    this->runnable = runnable;
    this->mutex = new std::mutex(); 
    this->completed_mutex = new std::mutex();
    this->completed = new std::condition_variable();
    this->deps = deps;
    this->is_completed = false;
    this->is_popped = false;
}

Task::~Task()
{
    delete this->runnable;
    delete this->mutex;
    delete this->completed_mutex;
    delete this->completed;
    // printf("[TasksQueue] Tasks destructed\n");
}

// Getter and setter methods

void Task::set_id(TaskID id)
{
    this->task_id = id;
}

TaskID Task::get_id()
{
    return this->task_id;
}

// Task methods
void Task::complete() {
    std::lock_guard<std::mutex> lock(*this->completed_mutex);
    this->is_completed = true;
    this->completed->notify_all();
}

void Task::wait() {
    std::unique_lock<std::mutex> lock(*this->completed_mutex);
    this->completed->wait(lock, [this] { return this->is_completed; });
}

void Task::run(int task_id) {
    this->runnable->runTask(task_id, this->num_tasks);
}

/*
 * ================================================================
 * TasksQueue Implementation
 * ================================================================
 */

TasksQueue::TasksQueue()
{
    this->counter = 0;
    this->done = false;
    this->tasks = new std::queue<Task*>();
    this->mutex = new std::mutex();
    this->has_tasks = new std::condition_variable();
}

TasksQueue::~TasksQueue()
{
    delete this->tasks;
    delete this->mutex;
    delete this->has_tasks;
    // printf("[TasksQueue] Queue destructed\n");
}

TaskID TasksQueue::next_id()
{
    std::lock_guard<std::mutex> lock(*this->mutex);
    return this->tasks->size();
}


Task* TasksQueue::front()
{   
    std::lock_guard<std::mutex> lock(*this->mutex);
    Task* task = this->tasks->front();
    return task;
}

void TasksQueue::remove_front()
{
    std::lock_guard<std::mutex> lock(*this->mutex);
    Task* task = this->tasks->front();
    this->tasks->pop();
    task->is_popped = true;
}

void TasksQueue::push_back(Task *task)
{
    std::lock_guard<std::mutex> lock(*this->mutex);
    if (task->get_id() == -1) {
        // floating task
        int id = this->counter;
        this->counter++;
        task->set_id(id);
    }
    // printf("[push_back] Queue push new task id %d with %d subtasks\n", task->get_id(), task->num_tasks);
    this->tasks->push(task);
    this->has_tasks->notify_all();
}


Task* TasksQueue::pop_front()
{
    // pop_front not actually removing the task from the queue, since Task has many subtasks
    std::unique_lock<std::mutex> lock(*this->mutex);
    this->has_tasks->wait(lock, [this] { 
        // if (this->tasks->empty() && !this->done) printf("[pop_front] Task queue is empty, waiting...\n");
        return (!this->tasks->empty() || this->done); 
    });
    Task* task = this->tasks->front();
    return task;
}

void TasksQueue::set_done()
{
    std::lock_guard<std::mutex> lock(*this->mutex);
    this->done = true;
    this->has_tasks->notify_all();
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
    // printf("[TaskSystemParallelThreadPoolSleeping] Constructing thread pool with %d threads\n", num_threads);
    // debug
    this->_debug_counter = 0;
    this->_debug_mutex = new std::mutex();

    this->tasks_queue = new TasksQueue();
    this->num_threads = num_threads;
    this->threads = new std::thread[num_threads];
    this->done = false;

    // Activate threads
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
    this->done = true;
    this->tasks_queue->set_done();

    for (int i = 0; i < this->num_threads; i++) {
        this->threads[i].join();
    }

    delete this->_debug_mutex;
    delete this->tasks_queue;
    delete[] this->threads;
    // printf("[TaskSystemParallelThreadPoolSleeping] Threads destructed all\n");

}


int TaskSystemParallelThreadPoolSleeping::taskExec(Task *task, int thread_id) {
    int total_tasks;
    int task_id = -1;

    // Check if there are tasks to run
    task->mutex->lock();
    total_tasks = task->num_tasks;
    task_id = total_tasks - task->num_left;

    if (task_id < total_tasks) {
        task->num_left--;
        task->mutex->unlock();

        task->run(task_id);
        // Sleep
        // std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // printf("[taskExec] Thread %d :: task %d with subtask %d is completed\n", thread_id, task->get_id(), task_id);
        task->mutex->lock();
        task->num_done++;

        if (task->num_done == total_tasks) {
            task->mutex->unlock();
            task->complete();
            // printf("[taskExec] Thread %d :: task %d before dequeue, queue size %d\n", thread_id, task->get_id(), this->tasks_queue->tasks->size());
            this->tasks_queue->remove_front();
            // printf("[taskExec] Thread %d :: task %d is all completed, queue size %lu\n", thread_id, task->get_id(), this->tasks_queue->tasks->size());
        }
        else {
            task->mutex->unlock();
        }
    } 
    else {
        task->mutex->unlock();
    }

    return task_id;
}


void TaskSystemParallelThreadPoolSleeping::threadFunc() {
    // TODO: implement this in the way that each task get a queue from TasksQueue if any, 
    // go to sleep if no task is available, and wake up when there is a task available
    this->_debug_mutex->lock();
    int _id = this->_debug_counter;
    this->_debug_counter++;
    this->_debug_mutex->unlock();
    Task* task;
    while (!this->done)
    {   
        bool can_run = true;

        // printf("[threadFunc] Thread %d spinning\n", _id);
        task = this->tasks_queue->pop_front();
        if (this->done) break;
        if (!task) continue;
        // printf("[threadFunc] Thread %d :: queue pop task %d\n", _id, task->get_id());
        
        for (auto dep : task->deps)
        {
            Task* dep_task = this->tasks_queue->front();
            if (dep_task && dep_task->get_id() == dep)
            {   
                // printf("[threadFunc] Thread %d :: dep %d is completed\n", _id, dep);
                if (!dep_task->is_completed)
                {
                    // printf("[threadFunc] Thread %d :: dep %d is not completed\n", _id, dep);
                    can_run = false;
                    break;
                }
            }
        }

        if (can_run) {
            this->taskExec(task, _id);
        }
        else {
            // printf("[threadFunc] Thread %d :: task %d cannot run, push back to queue\n", _id, task->get_id());
            this->tasks_queue->mutex->lock();
            bool is_task_popped = task->is_popped;
            if (!is_task_popped) this->tasks_queue->push_back(task);
            this->tasks_queue->mutex->unlock();
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
    auto task = new Task(runnable, num_total_tasks, deps);
    this->tasks_queue->push_back(task);
    return task->get_id();
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    while (true) {
        std::lock_guard<std::mutex> lock(*this->tasks_queue->mutex);
        if (this->tasks_queue->tasks->empty()) break;
    }
    printf("[sync] All tasks are completed\n");
    return;
}
