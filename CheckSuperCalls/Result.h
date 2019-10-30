#pragma once
#include "Defs.h"

struct Issue
{
	fs::path filepath;
	uint line;
	std::string funcname;
};

class Result
{
public:

	Result(int threads_num);

	virtual ~Result();

	using IssuesVec = std::vector<Issue*>;

	void AddIssue(Issue* issue, int thread_id);
	IssuesVec GetAllIssues() const;

protected:

	std::vector<IssuesVec> issues;
};