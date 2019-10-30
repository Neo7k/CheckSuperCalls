#pragma once
#include "Defs.h"

class Workers
{
public:

    Workers(int num_workers);
    virtual ~Workers();

    bool AreAllFree() const;

    using JobFunc = std::function<void(int thread_id, const fs::path&)>;

    void WaitUntilAllFree() const;
    void SetPaths(const FsPaths* paths);
    void DoJob(const JobFunc& func);

protected:

    enum class WorkerStatus : char
    {
        Free,
        Busy
    };

    static const int no_task = -1;

    struct Worker
    {
        std::thread* thread;
        WorkerStatus status = WorkerStatus::Free;
        int taks_id = no_task;
    };

    void WorkerFunc(int thread_id);
    void StartWorkers();

    std::vector<Worker> workers;
    JobFunc job;
    const FsPaths* tasks = nullptr;
    bool exit = false;
};