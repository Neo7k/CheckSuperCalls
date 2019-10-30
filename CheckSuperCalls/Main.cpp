#include "Config.h"
#include "Annex.h"
#include "Files.h"
#include "Code.h"
#include "Workers.h"
#include "Result.h"

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
	FsPaths paths;
	const size_t potential_files_count = 16384;
	paths.reserve(potential_files_count);

	Workers workers(config.GetNumThreads());
	workers.SetPaths(&paths);

	std::cout << "==============LOOKUP PHASE==============" << std::endl;
	Walk(CodeType::Header, config, [&](auto path)
	{
		paths.push_back(path);
	});

	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseLookup(content, path);
	});

	std::cout << "Files: " << paths.size() << std::endl;
	std::cout << "Classes: " << code.GetClassLookupSize() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
	t0 = timer.now();

	std::cout << "========BASE SEARCH PHASE===============" << std::endl;
	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseHeaderForBaseClasses(content);
	});
	std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Super Functions: " << code.GetCSFunctionsCount() << std::endl;
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
		if (ReadContent(path, content))
			code.ParseHeader(path, content, true);
	});
	std::cout << "Classes: " << code.GetClassesCount() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;
	t0 = timer.now();

	std::cout << "========SOURCE PARSE PHASE==============" << std::endl;
	paths.clear();
	Walk(CodeType::Source, config, [&](auto path)
	{
		paths.push_back(path);
	});

	Result result(config.GetNumThreads());

	workers.DoJob([&](int thread_index, auto& path)
	{
		std::string content;
		if (ReadContent(path, content))
			code.ParseCpp(thread_index, path, content, result);
	});

	std::cout << "Files: " << paths.size() << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t0).count() << "ms" << std::endl;

	std::cout << "===============RESULT====================" << std::endl;
	auto issues = result.GetAllIssues();
	for (auto& issue : issues)
		std::cout << issue->filepath.string() << "(" << issue->line << "): No super call in " << issue->funcname << std::endl;

	std::cout << "===============RESUME====================" << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(timer.now() - t00).count() << "ms" << std::endl;

	return 0;
}