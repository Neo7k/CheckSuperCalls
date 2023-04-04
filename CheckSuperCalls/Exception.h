#pragma once
#include <exception>
#include <string>
#include <stdio.h>
#include <stdarg.h>

class Exception : public std::runtime_error
{
public:

	Exception(const std::string& message)
		: runtime_error(message)
	{
	}
};

class ParseException : public Exception
{
public:

	ParseException(const fs::path& path, size_t pos, const std::string& message)
		: Exception(std::format("{}({}): ", path.string(), pos, message))
	{
	}
};
