#pragma once
#include "Defs.h"

struct Skip
{
	std::string dir;
	std::string file;
};

struct Parse
{
	std::filesystem::path dir;
	std::filesystem::path file;
	std::vector<Skip> skip;
};


class Config
{
public:

	bool ParseConfig(const std::filesystem::path& path);

	const std::vector<fs::path>& GetExt(CodeType type) const;
	const std::vector<Parse>& GetParseStructure() const;
	const fs::path& GetAnnexPath() const;

protected:

	fs::path GetPath(const std::filesystem::path& conf_path, const char* path_text) const;

	std::vector<fs::path> header_ext{".h", ".hpp"};
	std::vector<fs::path> source_ext{".cpp"};

	std::vector<Parse> parse;
	fs::path annex_path;
};