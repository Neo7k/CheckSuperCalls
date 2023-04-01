#pragma once
#include "Defs.h"
#include "Config.h"

class Config;

struct PathTs
{
	fs::path path;
	fs::file_time_type timestamp;
};

using PathTsVec = std::vector<PathTs>;

struct CodeFiles
{
	void Collect(const Config& config);

	const FsPaths& GetHeaders() const;
	const FsPaths& GetSource() const;

protected:

	FsPaths headers;
	FsPaths source;
};

bool ReadContent(const std::filesystem::path& path, std::string& content);
