#pragma once
#include "Defs.h"

class Workers
{
public:

	Workers(int num_workers);
	virtual ~Workers();

	using JobFunc = std::function<void(int thread_id, const fs::path&)>;

	void DoJob(const FsPaths* paths, JobFunc&& func);

protected:

	struct Worker
	{
		int task_id = 0;
		std::thread* thread;
		std::atomic<bool> busy = false;
	};

	void StartWorkers();

	bool AreAllFree() const;
	void WaitUntilAllFree();

	void WorkerFunc(int thread_id);

	Worker* workers = nullptr;
	int num_workers = 0;
	JobFunc job_func;
	const FsPaths* paths = nullptr;
	std::atomic<int> current_index = 0;
	std::atomic<bool> exit = false;

	std::condition_variable job_active_cv;
	std::mutex job_active_mutex;

	std::condition_variable worker_finished_cv;
	std::mutex worker_finished_mutex;
};