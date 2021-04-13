#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <set>
#include <array>
#include <cassert>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctype.h>
#include <fstream>
#include <functional>
#include <filesystem>

using string_vector = std::vector<std::string>;
using uint = unsigned int;
using uchar = unsigned char;
using int64 = int64_t;
namespace fs = std::filesystem;
using FsPaths = std::vector<fs::path>;

namespace std
{
	template<>
	struct hash<fs::path> : private hash<u8string>
	{
		size_t operator()(const fs::path& path) const
		{
			return hash<std::u8string>::operator()(path.u8string());
		}
	};
}

enum class CodeType
{
	Header,
	Source
};

namespace EKeywords
{
	enum TYPE : uint
	{
		Cur,            // {
		Ly,             // }
		Namespace,      // namespace
		Class,          // class
		Struct,         // struct
		Paren,          // (
		Thesis,         // )
		SColon,         // ;
		Colon,          // :
		Bra,            // <
		Ket,            // >
		CallSuper,      // call_super
		AttriBra,       // [[
		AttriKet,       // ]]
		Super,          // __super
		Friend,         // friend
		ComStart,       // /*
		ComEnd,         // */
		SCom,           // //
		EOL,            // \n
		Quotation,      // "
        SkipSuper,      // skip_super

        _First = Cur,
        _Last = SkipSuper,

		None = (uint)-1
	};
}

extern const char* g_Keywords[];
extern unsigned char g_KeywordsLengths[];

void InitKeywords();