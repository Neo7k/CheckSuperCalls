#pragma once
#include "Defs.h"

struct FuncName
{
	std::string name;
	string_vector namespase;

	bool operator == (const FuncName& other) const
	{
		if (namespase.size() != other.namespase.size())
			return false;

		for (uint i = 0; i < namespase.size(); ++i)
			if (namespase[i] != other.namespase[i])
				return false;

		return name == other.name;
	}
};

struct Issue
{
	fs::path file;
	uint line;
	FuncName funcname;
	FuncName root_funcname;
};

struct CodeFiles;

class Result
{
public:

	Result(int threads_num);

	virtual ~Result();

	using IssuesVec = std::vector<Issue*>;

	void AddIssue(Issue* issue, int thread_id);
	IssuesVec GetAllIssues() const;

	void Clear();

protected:

	std::vector<IssuesVec> issues;
};