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
    {
        std::cerr << "Usage: CheckSuperCalls {path to code|path to .paths file}" << std::endl;
        return 1;
    }

    InitKeywords();

	std::chrono::system_clock timer;
	auto t00 = timer.now();
	auto t0 = timer.now();

	Config config;

	Annex annex;
	annex.Parse(config.GetAnnexPath());

	Code code;
	const uint num_threads = std::thread::hardware_concurrency();
	Result result(num_threads);
	CodeFiles files(fs::path(argv[1]), config);

	t0 = timer.now();

	Workers workers(num_threads);
	workers.SetPaths(&files.GetHeaders());

#if (VERBOSE)
	std::cout << "==============LOOKUP PHASE==============" << std::endl;
#endif

	workers.DoJob([&code](int thread_index, auto& path)
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

    workers.DoJob([&code](int thread_index, auto& path)
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

    workers.DoJob([&code](int thread_index, auto& path)
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

	workers.SetPaths(&files.GetSource());
	workers.DoJob([&code, &result](int thread_index, auto& path)
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
		std::cout << fs::canonical(issue->file).string() << "(" << issue->line << "): No super call in " << DecorateWithNamespace(issue->funcname.name, issue->funcname.namespase) << std::endl;

#if (VERBOSE)
    std::cout << "===============RESUME====================" << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t00).count() << "ms" << std::endl;
#endif

	return 0;
}