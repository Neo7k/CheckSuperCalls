#pragma once
#include <exception>
#include <string>
#include <stdio.h>
#include <stdarg.h>

class Exception : public std::exception
{
public:

	Exception(const char* format, ...)
	{
		const size_t buf_size = 2048;
		char buf[buf_size];
		va_list args;
		va_start(args, format);
		snprintf(buf, buf_size, format, args);
		va_end(args);
		message = buf;
	}

	const char* what() const throw() override 
	{
		return message.c_str();
	}

protected:

	std::string message;
};

class ParseException : public Exception
{
public:

	template<typename... Values> ParseException(size_t pos, const char* format, Values... data)
		: Exception(format, data...)
		, parse_pos(pos)
	{
	}

	size_t GetPos() const
	{
		return parse_pos;
	}

protected:

	size_t parse_pos;
};

