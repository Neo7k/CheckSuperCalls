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

	for (auto is : cached_issues)
		grouped_result.push_back(is);

	return grouped_result;
}

void Result::ReadCache(std::ifstream& f)
{
	size_t sz;
	Deserialize(f, sz);
	for (size_t i = 0; i < sz; ++i)
	{
		auto issue = new Issue;
		Deserialize(f, issue->file);
		Deserialize(f, issue->line);
		Deserialize(f, issue->funcname.name);
		Deserialize(f, issue->funcname.namespase);
		Deserialize(f, issue->root_funcname.name);
		Deserialize(f, issue->root_funcname.namespase);
		cached_issues.push_back(issue);
	}
}

void Result::WriteCache(std::ofstream& f) const
{
	auto issues = GetAllIssues();
	Serialize(f, issues.size());
	for (auto& issue : issues)
	{
		Serialize(f, issue->file);
		Serialize(f, issue->line);
		Serialize(f, issue->funcname.name);
		Serialize(f, issue->funcname.namespase);
		Serialize(f, issue->root_funcname.name);
		Serialize(f, issue->root_funcname.namespase);
	}
}

void Result::UpdateCachedData(const CodeFiles& code_files)
{
	std::map<fs::path, std::vector<uint>> issues_by_file;

	for (uint i = 0; i < (uint)cached_issues.size(); ++i)
		issues_by_file[cached_issues[i]->file].push_back(i);

	for (auto& file : code_files.changed_source)
	{
		auto it = issues_by_file.find(file);
		if (it == issues_by_file.end())
			continue;

		for (auto i : it->second)
		{
			delete cached_issues[i];
			cached_issues[i] = nullptr;
		}
	}

	Erase(cached_issues, nullptr);
}

void Result::Clear()
{
	for (auto& is_v : issues)
	{
		for (auto is : is_v)
			delete is;

		is_v.clear();
	}

	for (auto& is : cached_issues)
		delete is;

	cached_issues.clear();
}

void Result::EraseCachedIssues(const FuncName& root_func)
{
	auto it = FindIf(cached_issues, [&root_func](auto issue) {return issue->root_funcname == root_func;});
	if (it != cached_issues.end())
	{
		delete *it;
		*it = nullptr;
	}

	Erase(cached_issues, nullptr);
}
