#include "Config.h"
#include "Annex.h"
#include "Files.h"
#include "Code.h"
#include "Workers.h"
#include "Result.h"
#include "Strings.h"
#include "Exception.h"

int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: CheckSuperCalls {path to code|path to .paths file} [path to .config]" << std::endl;
		return 1;
	}

	try
	{
		InitKeywords();

		std::chrono::system_clock timer;
		auto t00 = timer.now();
		auto t0 = timer.now();

		Config config;
		if (argc > 2)
			config = Config(argv[2]);

		Annex annex;

		Code code;
		const uint num_threads = std::thread::hardware_concurrency();
		Result result(num_threads);

		t0 = timer.now();

		if (config.GetVerbosity() > 0)
		{
			std::cout << "==================WALK==================" << std::endl;
		}

		CodeFiles files(fs::path(argv[1]), config);

		if (config.GetVerbosity() > 0)
		{
			std::cout << "Header files: " << files.GetHeaders().size() << std::endl;
			std::cout << "Source files: " << files.GetSource().size() << std::endl;
			std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
		}

		t0 = timer.now();

		Workers workers(num_threads);
		workers.SetPaths(&files.GetHeaders());

		if (config.GetVerbosity() > 0)
		{
			std::cout << "=================LOOKUP=================" << std::endl;
		}

		workers.DoJob([&code](int thread_index, auto& path)
					  {
						  std::string content;
						  if (ReadContent(path, content))
							  code.ParseLookup(path, content);
					  });

		if (config.GetVerbosity() > 0)
		{
			std::cout << "Classes: " << code.GetClassLookupSize() << std::endl;
			std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
		}

		t0 = timer.now();

		if (config.GetVerbosity() > 0)
		{
			std::cout << "===============BASE SEARCH==============" << std::endl;
		}

		workers.DoJob([&code](int thread_index, auto& path)
					  {
						  std::string content;
						  if (ReadContent(path, content))
							  code.ParseHeaderForBaseClasses(path, content);
					  });

		code.UpdateAllCallSupers();

		if (config.GetVerbosity() > 0)
		{
			std::cout << "Classes: " << code.GetClassesCount() << std::endl;
			std::cout << "Super Functions: " << code.GetCSFunctionsCount() << std::endl;
		}

		if (config.GetVerbosity() > 0)
		{
			std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
		}

		t0 = timer.now();

		if (config.GetVerbosity() > 0)
		{
			std::cout << "===============ANNEX APPLY==============" << std::endl;
		}

		code.ApplyAnnex(annex);

		if (config.GetVerbosity() > 0)
		{
			std::cout << "Classes: " << code.GetClassesCount() << std::endl;
			std::cout << "Super Functions: " << code.GetCSFunctionsCount() << std::endl;
		}

		if (config.GetVerbosity() > 0)
		{
			std::cout << "==============HEADER PARSE==============" << std::endl;
		}

		workers.DoJob([&code](int thread_index, auto& path)
					{
						  std::string content;
						  FsPaths paths;
						  if (ReadContent(path, content))
							  code.ParseHeader(path, content, true, paths);
					});

		if (config.GetVerbosity() > 0)
		{
			std::cout << "Classes: " << code.GetClassesCount() << std::endl;
			std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
		}

		t0 = timer.now();

		if (config.GetVerbosity() > 0)
		{
			std::cout << "==============SOURCE PARSE==============" << std::endl;
		}

		workers.SetPaths(&files.GetSource());
		workers.DoJob([&code, &result](int thread_index, auto& path)
					  {
						  std::string content;
						  if (ReadContent(path, content))
							  code.ParseCpp(thread_index, path, content, result);
					  });

		if (config.GetVerbosity() > 0)
		{
			std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
		}

		if (config.GetVerbosity() > 0)
		{
			std::cout << "=================RESULT=================" << std::endl;
		}

		auto issues = result.GetAllIssues();
		for (auto& issue : issues)
			std::cout << fs::canonical(issue->file).string() << "(" << issue->line << "): No super call in " << DecorateWithNamespace(issue->funcname.name, issue->funcname.namespase) << std::endl;

		if (config.GetVerbosity() > 0)
		{
			std::cout << "=================RESUME=================" << std::endl;
			std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t00).count() << "ms" << std::endl;
		}
	}
	catch (const std::exception& except)
	{
		std::cerr << except.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown exception" << std::endl;
		return 1;
	}

	return 0;
}