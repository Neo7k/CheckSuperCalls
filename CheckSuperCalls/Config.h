#pragma once
#include "Defs.h"

class Config
{
public:

	Config() = default;
	Config(const fs::path& path);

	const std::vector<fs::path>& GetExt(CodeType type) const;
	int GetVerbosity() const;

protected:

	std::vector<fs::path> header_ext{".h"};
	std::vector<fs::path> source_ext{".cpp"};
	int verbosity = 0;
};