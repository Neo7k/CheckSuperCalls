#pragma once
#include "Defs.h"

class Config
{
public:

	Config() = default;
	Config(const fs::path& path);

	const std::vector<fs::path>& GetExt(CodeType type) const;
	int GetVerbosity() const;
	const char* GetCallSuperKeyword() const;
	const char* GetSkipSuperKeyword() const;

protected:

	std::vector<fs::path> header_ext{".h"};
	std::vector<fs::path> source_ext{".cpp"};
	int verbosity = 0;
	std::string call_super_keyword = "csc::call_super";
	std::string skip_super_keyword = "csc::skip_super";
};