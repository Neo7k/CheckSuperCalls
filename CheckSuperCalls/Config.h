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

	const string_vector& GetExt(CodeType type) const;
	const std::vector<Parse>& GetParseStructure() const;
	int GetNumThreads() const;
	const fs::path& GetCachePath() const;
	const fs::path& GetAnnexPath() const;

protected:

	fs::path GetPath(const std::filesystem::path& conf_path, const char* path_text) const;

	string_vector header_ext;
	string_vector source_ext;

	std::vector<Parse> parse;
	int num_threads = 1;
	fs::path annex_path;
	fs::path cache_path;
};