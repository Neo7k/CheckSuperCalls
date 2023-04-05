#pragma once
#include "Defs.h"

struct find_result
{
	size_t pos = -1;
	size_t pos_end = -1;
	EKeywords::TYPE key = EKeywords::None;
	int extra_index = -1;

	explicit operator bool() const
	{
		return pos != (size_t)-1;
	}
};

inline bool find_first_impl(const char* str, const char* keyword, unsigned char keyword_length, bool positive_pos)
{
	if (str[0] != keyword[0])
		return false;

	size_t j = 1;
	for (; j < keyword_length; ++j)
		if (str[j] != keyword[j])
			return false;

	if ((unsigned char)str[j] > 127u)
		return false;

	if (isalnum(str[0]) && (isalnum(str[j]) || str[j] == '_'))
		return false;

	if (positive_pos && isalnum(str[0]) && (isalnum(str[-1]) || str[-1] == '_'))
		return false;

	return true;
}

template<typename KeysArray>
find_result find_first(const std::string& str, size_t pos, const KeysArray& key_indexes)
{
	find_result result;
	size_t strln = str.length();
	auto c_str = str.c_str();

	for (size_t i = pos; i < strln; ++i)
	{
		for (uint k = 0; k < key_indexes.size(); ++k)
		{
			uint key_index = key_indexes[k];
			unsigned char key_len = g_KeywordsLengths[key_index];
			if (strln - i < key_len)
				continue;

			if (find_first_impl(c_str + i, g_Keywords[key_index], key_len, i > 0))
				return find_result{ i, i + key_len, (EKeywords::TYPE)key_index };
		}
	}

	return find_result();
}

template<typename KeysArray>
find_result find_first(const std::string& str, size_t pos, const KeysArray& key_indexes, const string_vector& extras)
{
	find_result result;
	size_t strln = str.length();
	auto c_str = str.c_str();

	for (size_t i = pos; i < strln; ++i)
	{
		for (uint k = 0; k < key_indexes.size(); ++k)
		{
			uint key_index = key_indexes[k];
			unsigned char key_len = g_KeywordsLengths[key_index];
			if (strln - i < key_len)
				continue;

			if (find_first_impl(c_str + i, g_Keywords[key_index], key_len, i > 0))
				return find_result{ i, i + key_len, (EKeywords::TYPE)key_index };
		}

		int esize = (int)extras.size();
		for (int j = 0; j < esize; ++j)
		{
			unsigned char extra_len = (unsigned char)extras[j].length();
			if (strln - i < extra_len)
				continue;

			if (find_first_impl(c_str + i, extras[j].c_str(), extra_len, i > 0))
				return find_result{ i, i + extra_len, EKeywords::None, j };
		}
	}

	return find_result();
}

template<typename KeysArray>
find_result find_first_reverse(const std::string& str, size_t pos, const KeysArray& key_indexes)
{
	find_result result;
	size_t strln = str.length();
	auto c_str = str.c_str();

	for (size_t i = pos; i != (size_t)-1; --i)
	{
		for (uint k = 0; k < key_indexes.size(); ++k)
		{
			uint key_index = key_indexes[k];
			unsigned char key_len = g_KeywordsLengths[key_index];
			if (strln - i < key_len)
				continue;

			if (find_first_impl(c_str + i, g_Keywords[key_index], key_len, i > 0))
				return find_result{ i, i + key_len, (EKeywords::TYPE)key_index };
		}
	}

	return find_result();
}

std::string TrimSpaces(const char* from, const char* to);

struct SuperClassName
{
	std::string name;
	string_vector namespase;
};

void ParseInheritance(const char* from, const char* to, std::vector<SuperClassName>& super_classes);
std::string GetName(const char* from, bool reverse = false);
void ParseNameWithNamespaceBackwards(const char* from, std::string& name, string_vector& namespase, const char* to = nullptr);
std::string ParseClassNameBackwards(const char* from, const char* to);
std::string DecorateWithNamespace(const std::string& name, const string_vector& namespase);
void NormalizeLineEndings(std::string& text);
uint GetLineIndex(const std::string& str, size_t pos);
bool strcmp_range(const char* from, const char* to, const char* what);
std::vector<std::string_view> Tokenize(std::string_view str, char token, uint max_elems = std::numeric_limits<uint>::max());
