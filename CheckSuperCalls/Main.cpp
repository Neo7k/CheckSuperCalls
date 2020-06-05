#include "Config.h"
#include "Annex.h"
#include "Files.h"
#include "Code.h"
#include "Workers.h"
#include "Result.h"
#include "Strings.h"

#define VERBOSE 0

int main(int argc, const char* argv[])
{
	if (argc < 2)
		return 1;

	std::chrono::system_clock timer;
	auto t00 = timer.now();
	auto t0 = timer.now();

	Config config;
	if (!config.ParseConfig(argv[1]))
		return 1;

	Annex annex;
	annex.Parse(config.GetAnnexPath());

	Code code;
	Result result(config.GetNumThreads());
	CodeFiles files;

#if (VERBOSE)
	std::cout << "==============CACHE PHASE==============" << std::endl;
#endif

	t0 = timer.now();
	{
		std::ifstream f(config.GetCachePath(), std::ios::binary);
		if (f)
		{
			files.ReadCache(f);
			code.ReadCache(f);
			result.ReadCache(f);
		}
	}

	const uint possible_files_count = 16384; // cuz why not
	PathTsVec header_files; header_files.reserve(possible_files_count);
	PathTsVec source_files; source_files.reserve(possible_files_count);

	Walk(CodeType::Header, config, [&](auto& path) 
	{
		header_files.emplace_back(PathTs{path, fs::last_write_time(path)});
	});

	Walk(CodeType::Source, config, [&](auto& path) 
	{
		source_files.emplace_back(PathTs{path, fs::last_write_time(path)});
	});

	files.SyncCacheToFiles(header_files, source_files);
	code.UpdateCachedData(files);
	result.UpdateCachedData(files);

#if (VERBOSE)
	std::cout << "Files: " << std::endl;
	std::cout << "|--overall: " << header_files.size() + source_files.size() << std::endl;
	std::cout << "|--changed: " << files.changed_headers.size() + files.changed_source.size() << std::endl;
	std::cout << "|--deleted: " << files.deleted_files.size() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
	t0 = timer.now();
#endif

	Workers workers(config.GetNumThreads());
	workers.SetPaths(&files.changed_headers);

#if (VERBOSE)
	std::cout << "==============LOOKUP PHASE==============" << std::endl;
#endif

	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseLookup(path, content);
	});

#if (VERBOSE)
	std::cout << "Classes: " << code.GetClassLookupSize() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
#endif
	
    t0 = timer.now();

#if (VERBOSE)
    std::cout << "========BASE SEARCH PHASE===============" << std::endl;
#endif

    workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseHeaderForBaseClasses(path, content);
	});
	
	code.UpdateAllCallSupers();
	
#if (VERBOSE)
    std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Super Functions: " << code.GetCSFunctionsCount() << std::endl;
#endif

    if (code.DidCSFunctionsChange())
	{
		
#if (VERBOSE)
        std::cout << "Super Functions have changed:" << std::endl;
		std::cout << "|--added: " << code.GetCSFunctionsAdded().size() << std::endl;
		std::cout << "|--deleted: " << code.GetCSFunctionsRemoved().size() << std::endl;
#endif

		if (!code.GetCSFunctionsAdded().empty())
		{
#if (VERBOSE)
            std::cout << "... performing full rebuild" << std::endl;
#endif
            files.MarkAllChanged();
			result.Clear();
		}
		else if (!code.GetCSFunctionsRemoved().empty())
		{
#if (VERBOSE)
            std::cout << "... cleaning removed functions from the result cache" << std::endl;
#endif
            // TODO:
			//result.EraseCachedIssues(code.GetCSFunctionsRemoved());
		}
	}

#if (VERBOSE)
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
#endif

    t0 = timer.now();

#if (VERBOSE)
    std::cout << "===========ANNEX APPLY PHASE============" << std::endl;
#endif

    code.ApplyAnnex(annex);

#if (VERBOSE)
    std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Super Functions: " << code.GetCSFunctionsCount() << std::endl;
#endif

#if (VERBOSE)
    std::cout << "=========HEADER PARSE PHASE=============" << std::endl;
#endif

    workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		FsPaths paths;
		if (ReadContent(path, content))
			code.ParseHeader(path, content, true, paths);
	});

#if (VERBOSE)
    std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
#endif
    
    t0 = timer.now();

#if (VERBOSE)
    std::cout << "========SOURCE PARSE PHASE==============" << std::endl;
#endif

	workers.SetPaths(&files.changed_source);
	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseCpp(thread_index, path, content, result);
	});

#if (VERBOSE)
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
#endif

#if (VERBOSE)
    std::cout << "===============RESULT====================" << std::endl;
#endif

    auto issues = result.GetAllIssues();
	for (auto& issue : issues)
		std::cout << issue->file.string() << "(" << issue->line << "): No super call in " << DecorateWithNamespace(issue->funcname.name, issue->funcname.namespase) << std::endl;

#if (VERBOSE)
    std::cout << "===============RESUME====================" << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t00).count() << "ms" << std::endl;
#endif

	{
		std::ofstream f(config.GetCachePath(), std::ios::binary);
		if (f)
		{
			files.WriteCache(f);
			code.WriteCache(f);
			result.WriteCache(f);
		}
	}

	return 0;
}