#include "Result.h"
#include "Files.h"
#include "Algorithms.h"

Result::Result(int threads_num)
{
	issues.resize(threads_num);
}

Result::~Result()
{
	Clear();
}

void Result::AddIssue(Issue* issue, int thread_id)
{
	issues[thread_id].push_back(issue);
}

Result::IssuesVec Result::GetAllIssues() const
{
	IssuesVec grouped_result;
	for (auto& is_v : issues)
		for (auto is : is_v)
			grouped_result.push_back(is);

	return grouped_result;
}

void Result::Clear()
{
	for (auto& is_v : issues)
	{
		for (auto is : is_v)
			delete is;

		is_v.clear();
	}
}
