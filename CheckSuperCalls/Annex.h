#pragma once
#include "Defs.h"

struct AnnexClass
{
	string_vector namespase;
	std::string name;
	string_vector functions;
};

struct Annex
{
	bool Parse(const fs::path& path);

	std::vector<AnnexClass> classes;
};