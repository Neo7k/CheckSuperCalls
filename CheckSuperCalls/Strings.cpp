#include "Strings.h"

std::string TrimSpaces(const char* from, const char* to)
{
	std::string result;
	result.reserve(to - from);
	for (auto pc = from; pc != to; ++pc)
	{
		char c = *pc;
		if (c != ' ' && c != '\t' && c != '\n')
			result.push_back(c);
	}

	return result;
}

uint GetLineIndex(const std::string& str, size_t pos)
{
	uint num_lines = 0;
	for (size_t i = pos; i != (size_t)-1; --i)
		if (str[i] == '\n')
			++num_lines;

	return num_lines + 1;
}

bool strcmp_range(const char* from, const char* to, const char* what)
{
	auto pc = from;
	auto pc2 = what;
	for (; pc != to; ++pc, ++pc2)
	{
		if (*pc != *pc2)
			return false;
	}

	return true;
}

void ParseInheritance(const char* from, const char* to, std::vector<SuperClassName>& super_classes)
{
	const char* publik = "public"; const size_t publik_size = 6;
	const char* protekted = "protected"; const size_t protekted_size = 9;
	const char* priwate = "private"; const size_t priwate_size = 7;


	std::string str;
	str.reserve(to - from);
	auto pc = from;
	super_classes.emplace_back();

	while (pc < to)
	{
		char c = *pc;
		if (c != ' ' && c != '\t' && c != '\n')
		{
			if (c == 'p')
			{
				if (strcmp_range(pc, pc + publik_size, publik))
				{
					pc += publik_size;
					continue;
				}
				else if (strcmp_range(pc, pc + protekted_size, protekted))
				{
					pc += protekted_size;
					continue;
				}
				else if (strcmp_range(pc, pc + priwate_size, priwate))
				{
					pc += priwate_size;
					continue;
				}
			}
			else if (c == ':')
			{
				super_classes.back().namespase.push_back(str);
				str.clear();
				pc += 2;
				continue;
			}
			else if (c == ',')
			{
				++pc;
				if (!str.empty())
				{
					super_classes.back().name = str;
					str.clear();
				}
				super_classes.emplace_back();
				continue;
			}
			else if (c == '<')
			{
				while (pc != to && *pc != '>')
					++pc;
				++pc;
				continue;
			}
			else if (c == '/')
			{
				if (!str.empty())
				{
					super_classes.back().name = str;
					str.clear();
				}
				if (pc[1] == '/')
				{
					while (pc != to && *pc != '\n')
						++pc;
					++pc;
					continue;
				}
				else if (pc[1] == '*')
				{
					while (pc != to && !(pc[0] == '*' && pc[1] == '/'))
						++pc;
					pc += 2;
					continue;
				}
			}

			str.push_back(c);
		}
		else
		{
			if (!str.empty())
			{
				super_classes.back().name = str;
				str.clear();
			}
		}

		++pc;
	}
}

std::string GetName(const char* from, bool reverse/* = false*/)
{
	std::string result;
	result.reserve(32);
	int inc_dec = reverse ? -1 : 1;
	bool recording = false;
	for (auto pc = from;; pc += inc_dec)
	{
		char c = *pc;
		if (c == ' ' || c == '\t' || c == '\n')
		{
			if (!recording)
				continue;
			else
				break;
		}
		else
		{
			recording = true;
			result.push_back(c);
		}
	}

	if (reverse)
		std::reverse(result.begin(), result.end());

	return result;
}

void ParseNameWithNamespaceBackwards(const char* from, std::string& name, string_vector& namespase, const char* to/* = nullptr*/)
{
	auto p = from;
	auto pstr = &name;
	while (true)
	{
		char c = *p;
		auto& s = *pstr;
		if (c == ' ' || c == '\t' || c == '\n' || p == to)
		{
			std::reverse(s.begin(), s.end());
			std::reverse(namespase.begin(), namespase.end());
			return;
		}

		if (c == ':')
		{
			p -= 2;
			std::reverse(s.begin(), s.end());
			pstr = &namespase.emplace_back();
			continue;
		}

		s.push_back(c);
		--p;
	}
}

std::string ParseClassNameBackwards(const char* from, const char* to)
{
	std::string name;
	name.reserve(from - to);
	auto p = from;
	bool capturing = false;
	int template_lvl = 0;
	while (p != to)
	{
		char c = *p;
		if (c == ' ' || c == '\t' || c == '\n')
		{
			if (capturing)
			{
				std::reverse(name.begin(), name.end());
				return name;
			}
		}
		else if (c == '>')
		{
			++template_lvl;
			name.push_back(c);
		}
		else if (c == '<')
		{
			--template_lvl;
			name.push_back(c);
		}
		else
		{
			if (template_lvl == 0)
				capturing = true;
			name.push_back(c);
		}
		--p;
	}
	std::reverse(name.begin(), name.end());
	return name;
}

std::string DecorateWithNamespace(const std::string& name, const string_vector& namespase)
{
	std::string result;
	for (auto& ns : namespase)
	{
		result += ns + "::";
	}

	return result + name;
}

void NormalizeLineEndings(std::string& text)
{
	if (text.empty())
		return;

	auto p = &text[0];
	auto e = p + text.length() - 1;
	for (; p != e; ++p)
		if (p[0] == '\r')
			if (p[1] == '\n')
				p[0] = ' ';
			else
				p[0] = '\n';

}
