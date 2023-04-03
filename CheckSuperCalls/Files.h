#pragma once
#include "Defs.h"
#include "Config.h"

class Config;

struct CodeFiles
{
	CodeFiles(const fs::path& path, const Config& config);

	const FsPaths& GetHeaders() const;
	const FsPaths& GetSource() const;

protected:

	


	FsPaths headers;
	FsPaths source;
};

bool ReadContent(const std::filesystem::path& path, std::string& content);
