#include "Defs.h"

const char* g_Keywords[EKeywords::_Last - EKeywords::_First + 1] =
{
	"{",
	"}",
	"namespace",
	"class",
	"struct",
	"(",
	")",
	";",
	":",
	"<",
	">",
	"csc::call_super",
	"[[",
	"]]",
	"__super",
	"friend",
	"/*",
	"*/",
	"//",
	"\n",
	"\"",
    "csc::skip_super"
};

unsigned char g_KeywordsLengths[EKeywords::_Last - EKeywords::_First + 1];

void InitKeywords()
{
    for (uint i = EKeywords::_First; i <= EKeywords::_Last; ++i)
    {
        g_KeywordsLengths[i] = (unsigned char)strlen(g_Keywords[i]);
    }
}