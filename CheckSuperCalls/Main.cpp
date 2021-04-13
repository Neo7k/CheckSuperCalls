#include "Config.h"
#include "Annex.h"
#include "Files.h"
#include "Code.h"
#include "Workers.h"
#include "Result.h"
#include "Strings.h"

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

	std::cout << "==============CACHE PHASE==============" << std::endl;
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

	std::cout << "Files: " << std::endl;
	std::cout << "|--overall: " << header_files.size() + source_files.size() << std::endl;
	std::cout << "|--changed: " << files.changed_headers.size() + files.changed_source.size() << std::endl;
	std::cout << "|--deleted: " << files.deleted_files.size() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
	t0 = timer.now();

	Workers workers(config.GetNumThreads());
	workers.SetPaths(&files.changed_headers);

	std::cout << "==============LOOKUP PHASE==============" << std::endl;
	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseLookup(path, content);
	});

	std::cout << "Classes: " << code.GetClassLookupSize() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
	t0 = timer.now();

	std::cout << "========BASE SEARCH PHASE===============" << std::endl;
	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseHeaderForBaseClasses(path, content);
	});
	
	code.UpdateAllCallSupers();
	
	std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Super Functions: " << code.GetCSFunctionsCount() << std::endl;
	if (code.DidCSFunctionsChange())
	{
		
		std::cout << "Super Functions have changed:" << std::endl;
		std::cout << "|--added: " << code.GetCSFunctionsAdded().size() << std::endl;
		std::cout << "|--deleted: " << code.GetCSFunctionsRemoved().size() << std::endl;

		if (!code.GetCSFunctionsAdded().empty())
		{
			std::cout << "... performing full rebuild" << std::endl;
			files.MarkAllChanged();
			result.Clear();
		}
		else if (!code.GetCSFunctionsRemoved().empty())
		{
			std::cout << "... cleaning removed functions from the result cache" << std::endl;
			// TODO:
			//result.EraseCachedIssues(code.GetCSFunctionsRemoved());
		}
	}
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
	t0 = timer.now();

	std::cout << "===========ANNEX APPLY PHASE============" << std::endl;
	code.ApplyAnnex(annex);
	std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Super Functions: " << code.GetCSFunctionsCount() << std::endl;

	std::cout << "=========HEADER PARSE PHASE=============" << std::endl;
	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		FsPaths paths;
		if (ReadContent(path, content))
			code.ParseHeader(path, content, true, paths);
	});
	std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
	t0 = timer.now();

	std::cout << "========SOURCE PARSE PHASE==============" << std::endl;

	workers.SetPaths(&files.changed_source);
	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseCpp(thread_index, path, content, result);
	});

	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;

	std::cout << "===============RESULT====================" << std::endl;
	auto issues = result.GetAllIssues();
	for (auto& issue : issues)
		std::cout << fs::canonical(issue->file).string() << "(" << issue->line << "): No super call in " << DecorateWithNamespace(issue->funcname.name, issue->funcname.namespase) << std::endl;

	std::cout << "===============RESUME====================" << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t00).count() << "ms" << std::endl;

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