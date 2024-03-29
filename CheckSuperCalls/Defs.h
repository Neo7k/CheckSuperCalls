#pragma once
#include <string>
#include <cstring>
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
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <ctype.h>
#include <fstream>
#include <functional>
#include <filesystem>
#include <charconv>
#include <ranges>

using string_vector = std::vector<std::string>;
using uint = unsigned int;
using uchar = unsigned char;
using int64 = int64_t;
namespace fs = std::filesystem;
using FsPaths = std::vector<fs::path>;

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
