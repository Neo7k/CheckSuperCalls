#include "Workers.h"
#include "Exception.h"

std::chrono::milliseconds wait_time(10);

Workers::Workers(int num_workers)
{
    workers.resize(num_workers);
    for (int i = 0; i < num_workers; i++)
    {
        auto thread = new std::thread([this, i] () 
        {
            WorkerFunc(i);
        });

        workers[i].thread = thread;
    }
}

Workers::~Workers()
{
    exit = true;
    for (auto& worker : workers)
        worker.thread->join();

    for (auto& worker : workers)
        delete worker.thread;
}

bool Workers::AreAllFree() const
{
    for (auto& w : workers)
        if (w.status != WorkerStatus::Free)
            return false;

    return true;
}

void Workers::DoJob(const JobFunc& func)
{
    WaitUntilAllFree();
    job = func;
    StartWorkers();
    WaitUntilAllFree();
}

void Workers::SetPaths(const FsPaths* paths)
{
    tasks = paths;
}

void Workers::WaitUntilAllFree() const
{
    while (!AreAllFree())
        std::this_thread::sleep_for(wait_time);
}

void Workers::WorkerFunc(int thread_id)
{
    while (!exit)
    {
        if (workers[thread_id].status == WorkerStatus::Free)
        {
            std::this_thread::sleep_for(wait_time);
            continue;
        }

        uint task_id = workers[thread_id].taks_id;
        if (task_id == no_task)
            task_id = thread_id;
        else
            task_id += (int)workers.size();

        if (task_id < (int)tasks->size())
        {
            job(thread_id, (*tasks)[task_id]);
            workers[thread_id].taks_id = task_id;
        }
        else
            workers[thread_id].status = WorkerStatus::Free;
    }
}

void Workers::StartWorkers()
{
    for (auto& w : workers)
    {
        w.taks_id = no_task;
        w.status = WorkerStatus::Busy;
    }
}
