#include "Workers.h"
#include "Exception.h"

std::chrono::milliseconds wait_time(10);

Workers::Workers(int num_workers)
{
	this->num_workers = num_workers;
	workers = new Worker[num_workers];
	for (int i = 0; i < num_workers; i++)
	{
		auto thread = new std::thread([this, i]()
		{
			WorkerFunc(i);
		});

		workers[i].thread = thread;
	}
}

Workers::~Workers()
{
	exit = true;

	job_active_cv.notify_all();

	for (int i = 0; i < num_workers; i++)
		workers[i].thread->join();

	for (int i = 0; i < num_workers; i++)
		delete workers[i].thread;

	delete[] workers;
}

void Workers::DoJob(const FsPaths* paths, JobFunc&& func)
{
	WaitUntilAllFree();
	this->paths = paths;
	job_func = std::move(func);
	StartWorkers();
	WaitUntilAllFree();
}

void Workers::WaitUntilAllFree()
{
	std::unique_lock lk(worker_finished_mutex);
	worker_finished_cv.wait(lk, [this] {return AreAllFree();});
}

bool Workers::AreAllFree() const
{
	for (int i = 0; i < num_workers; i++)
		if (workers[i].busy)
			return false;

	return true;
}

void Workers::StartWorkers()
{
	current_index = 0;
	for (int i = 0; i < num_workers; i++)
	{
		workers[i].busy = true;
	}
	job_active_cv.notify_all();
}

void Workers::WorkerFunc(int thread_id)
{
	auto& worker = workers[thread_id];

	while (!exit)
	{
		{
			// Sleep until marked as busy
			std::unique_lock lock(job_active_mutex);
			job_active_cv.wait(lock, [&worker, this] {return worker.busy.load() || exit.load();});
		}

		while (worker.busy)
		{
			worker.task_id = current_index.fetch_add(1);
			if (paths && worker.task_id < paths->size())
			{
				job_func(thread_id, (*paths)[worker.task_id]);
			}
			else
			{
				worker.busy = false;
				worker_finished_cv.notify_one();
			}
		}
	}
}
