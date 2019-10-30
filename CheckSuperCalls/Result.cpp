#include "Result.h"

Result::Result(int threads_num)
{
	issues.resize(threads_num);
}

Result::~Result()
{
	for (auto& is_v : issues)
		for (auto is : is_v)
			delete is;
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