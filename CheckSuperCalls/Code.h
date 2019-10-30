#pragma once
#include "Class.h"
#include "Defs.h"

class Result;

class Code
{
public:

    virtual ~Code();

    void ParseLookup(const std::string& content, const fs::path& path);
    void ParseHeaderForBaseClasses(const std::string& content);
    void ParseHeader(const std::string& content, bool skip_if_no_target_funcs = true);
    void ParseCpp(int thread_id, const fs::path& path, const std::string& content, Result& results);

    size_t GetClassLookupSize() const;
    size_t GetClassesCount() const;
    size_t GetCSFunctionsCount() const;

protected:

    struct NamespaseStack
    {
        int level = 0;
        string_vector namespase;
        string_vector namespase_trunk; // same namespace which doesn't include the last name (probably class name)
        std::vector<int> ns_levels; // {} stack level at which namespace is defined

        void Push()
        {
            ++level;
        }

        void Push(std::string&& ns)
        {
            ++level;
            if (!namespase.empty())
                namespase_trunk.push_back(namespase.back());

            namespase.push_back(ns);
            ns_levels.push_back(level);
        }

        void Pop()
        {
            if (!ns_levels.empty() && ns_levels.back() == level)
            {
                namespase.pop_back();
                ns_levels.pop_back();
                if (!namespase_trunk.empty())
                    namespase_trunk.pop_back();
            }
            --level;
        }
    };

    Class* GetClass(const std::string& name, const string_vector& namespase, const string_vector& possible_namespase, bool thread_lock) const;
    Class* CreateClass(const std::string& name, const string_vector& namespase);
    const Class* FindFuncInSuper(const std::string func_name, const Class* clazz) const;

    void ParseNamespace(const std::string& content, size_t& pos, NamespaseStack& ns);
    bool IsComment(const std::string& content, size_t pos);
    void SkipMultilineComment(const std::string& content, size_t& pos);

    std::vector<Class*> classes;
    mutable std::mutex classes_mutex;
    string_vector functions_call_super;
    std::mutex functions_call_super_mutex;
    using fs_paths = std::set<fs::path>;
    std::unordered_map<std::string, fs_paths> classdef_lookup;
    std::mutex classdef_lookup_mutex;
};
