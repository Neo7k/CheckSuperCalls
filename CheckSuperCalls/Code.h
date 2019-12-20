#pragma once
#include "Defs.h"
#include "Files.h"

class Result;
struct Class;
struct Annex;

class Code
{
public:

	virtual ~Code();

	void UpdateCachedData(const CodeFiles& code_files);
	void ParseLookup(const fs::path& path, const std::string& content);
	void ParseHeaderForBaseClasses(const fs::path& path, const std::string& content);
	void ApplyAnnex(const Annex& annex);
	void UpdateAllCallSupers();
	void ParseHeader(const fs::path& path, const std::string& content, bool skip_if_no_target_funcs, FsPaths& parsed_paths);
	void ParseCpp(int thread_id, const fs::path& path, const std::string& content, Result& results);

	void ReadCache(std::ifstream& f);
	void WriteCache(std::ofstream& f) const;

	size_t GetClassLookupSize() const;
	size_t GetClassesCount() const;
	size_t GetCSFunctionsCount() const;

	bool DidCSFunctionsChange() const;
	const string_vector& GetCSFunctionsAdded() const;
	const string_vector& GetCSFunctionsRemoved() const;

protected:

	struct NamespaseStack
	{
		void Push();
		void Push(std::string&& ns);
		void Pop();

		int level = 0;
		string_vector namespase;
		string_vector namespase_trunk; // same namespace which doesn't include the last name (probably class name)
		std::vector<int> ns_levels; // {} stack level at which namespace is defined
	};

	Class* GetClass(const std::string& name, const string_vector& namespase, const string_vector& possible_namespase) const;
	Class* CreateClass(const std::string& name, const string_vector& namespase, const fs::path& file);
	const Class* FindFuncInSuper(const std::string func_name, const Class* clazz) const;
	void CollectAllCallSupers();


	void ParseNamespace(const std::string& content, size_t& pos, NamespaseStack& ns);
	bool IsComment(const std::string& content, size_t pos);
	void SkipMultilineComment(const std::string& content, size_t& pos);
	bool SkipComments(EKeywords::TYPE key, const std::string& content, size_t& pos);

	std::recursive_mutex classes_mutex;
	//{
		std::vector<Class*> classes;
	//}

	string_vector functions_call_super;
	string_vector functions_call_super_removed;
	string_vector functions_call_super_added;

	std::mutex classdef_lookup_mutex;
	//{
		std::unordered_map<std::string, FsPaths> classdef_lookup;
	//}
};
