#pragma once
#include "Defs.h"

struct Class
{
	std::string name;
	string_vector namespase;
	string_vector functions;
	std::vector<Class*> derived_classes;
	std::vector<Class*> super_classes;
	fs::path file;
};
