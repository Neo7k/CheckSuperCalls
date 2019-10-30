#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
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

using string_vector = std::vector<std::string>;
using uint = unsigned int;
using uchar = unsigned char;
namespace fs = std::filesystem;
using FsPaths = std::vector<fs::path>;

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

        None = (uint)-1
    };
}

extern const char* g_Keywords[];
extern unsigned char g_KeywordsLengths[];

