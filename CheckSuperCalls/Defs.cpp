const char* g_Keywords[] =
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
	"call_super",
	"[[",
	"]]",
	"__super",
	"friend",
	"/*",
	"*/",
	"//",
	"\n"
};

unsigned char g_KeywordsLengths[] =
{
	1,
	1,
	9,
	5,
	6,
	1,
	1,
	1,
	1,
	1,
	1,
	10,
	2,
	2,
	7,
	6,
	2,
	2,
	2,
	1
};