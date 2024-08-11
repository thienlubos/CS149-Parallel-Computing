#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <condition_variable>   
#include <mutex>
#include <queue>
#include <thread>
#include <vector>   

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

class Task {
    public:
        Task(IRunnable* runnable, int num_total_tasks, const std::vector<TaskID> &deps);
        ~Task();
        // variables
        int num_tasks;
        int num_done;
        int num_left;
        bool is_completed;
        bool is_popped;
        IRunnable *runnable;
        TaskID task_id;
        std::vector<TaskID> deps;
        std::mutex *mutex;
        std::mutex *completed_mutex;
        std::condition_variable *completed;
        // Getter, setter methods
        void set_id(TaskID id);
        TaskID get_id();
        // Task methods
        void complete();
        void wait();
        void run(int task_id);
};

class TasksQueue {
    public:
        int counter;
        bool done;
        std::queue<Task*> *tasks;
        std::mutex *mutex;
        std::condition_variable *has_tasks;
        TasksQueue();
        ~TasksQueue();
        TaskID next_id();
        Task* front();
        void remove_front();
        Task* pop_front();
        void push_back(Task *task);
        void set_done();
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
        // debug and logging
        std::mutex *_debug_mutex;
        int _debug_counter;
        TasksQueue *tasks_queue;
        int num_threads;
        std::thread *threads;
        std::mutex *completed_mutex;
        std::condition_variable *completed;
        bool done;
        int taskExec(Task *task, int thread_id);
        void threadFunc();
};

#endif
