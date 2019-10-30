#pragma once
#include "Defs.h"
#include <string>

struct Class
{
	std::string name;
	string_vector namespase;
	string_vector functions;
	std::vector<Class*> derived_classes;
	std::vector<Class*> super_classes;
};