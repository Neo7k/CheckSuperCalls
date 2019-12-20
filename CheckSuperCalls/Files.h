#pragma once
#include "Defs.h"

class Config;

struct PathTs
{
	fs::path path;
	fs::file_time_type timestamp;
};

using PathTsVec = std::vector<PathTs>;

struct CodeFiles
{
	FsPaths changed_headers;
	FsPaths changed_source;
	FsPaths deleted_files;

	void ReadCache(std::ifstream& f);
	void WriteCache(std::ofstream& f) const;

	void SyncCacheToFiles(const PathTsVec& paths_headers, const PathTsVec& paths_source);
	void MarkAllChanged();

protected:

	std::map<fs::path, fs::file_time_type> file_timestamps;
	FsPaths all_headers;
	FsPaths all_source;
};

void Walk(CodeType code_type, const Config& config, const std::function<void(const std::filesystem::path& path)>& functor);
bool ReadContent(const std::filesystem::path& path, std::string& content);

template <typename T>
inline void Serialize(std::ofstream& f, const T& x)
{
	f.write(reinterpret_cast<const char*>(&x), sizeof(x));
}

template <typename T>
inline void Deserialize(std::ifstream& f, T& x)
{
	f.read(reinterpret_cast<char*>(&x), sizeof(x));
}

template <typename TElem, typename TTraits, typename TAlloc>
inline void Serialize(std::ofstream& f, const std::basic_string<TElem, TTraits, TAlloc>& x)
{
	Serialize(f, x.size());
	f.write(reinterpret_cast<const char*>(&x[0]), x.size() * sizeof(TElem));
}

template <typename TElem, typename TTraits, typename TAlloc>
inline void Deserialize(std::ifstream& f, std::basic_string<TElem, TTraits, TAlloc>& x)
{
	size_t sz;
	Deserialize(f, sz);
	x.resize(sz);
	f.read(reinterpret_cast<char*>(&x[0]), x.size() * sizeof(TElem));
}

template <>
inline void Serialize<fs::file_time_type>(std::ofstream& f, const fs::file_time_type& x)
{
	Serialize(f, x.time_since_epoch().count());
}

template <>
inline void Deserialize<fs::file_time_type>(std::ifstream& f, fs::file_time_type& x)
{
	fs::file_time_type::rep val;
	Deserialize(f, val);
	x = fs::file_time_type(fs::file_time_type::duration(val));
}

template <>
inline void Serialize<fs::path>(std::ofstream& f, const fs::path& x)
{
	Serialize(f, x.u32string());
}

template <>
inline void Deserialize<fs::path>(std::ifstream& f, fs::path& x)
{
	std::u32string str;
	Deserialize(f, str);
	x = str;
}

template <typename T>
inline void Serialize(std::ofstream& f, const std::vector<T>& x)
{
	Serialize(f, x.size());
	for (auto& value : x)
		Serialize(f, value);
}

template <typename T>
inline void Deserialize(std::ifstream& f, std::vector<T>& x)
{
	size_t sz;
	Deserialize(f, sz);
	x.resize(sz);
	for (size_t i = 0; i < sz; ++i)
	{
		T value;
		Deserialize(f, value);
		x[i] = value;
	}
}

template <typename TKey, typename TValue>
inline void Serialize(std::ofstream& f, const std::map<TKey, TValue>& x)
{
	Serialize(f, x.size());
	for (auto&[key, value] : x)
	{
		Serialize(f, key);
		Serialize(f, value);
	}
}

template <typename TKey, typename TValue>
inline void Deserialize(std::ifstream& f, std::map<TKey, TValue>& x)
{
	size_t sz;
	Deserialize(f, sz);
	for (size_t i = 0; i < sz; ++i)
	{
		TKey key;
		Deserialize(f, key);
		TValue value;
		Deserialize(f, value);
		x[key] = value;
	}
}

template <typename TKey, typename TValue>
inline void Serialize(std::ofstream& f, const std::unordered_map<TKey, TValue>& x)
{
	Serialize(f, x.size());
	for (auto&[key, value] : x)
	{
		Serialize(f, key);
		Serialize(f, value);
	}
}

template <typename TKey, typename TValue>
inline void Deserialize(std::ifstream& f, std::unordered_map<TKey, TValue>& x)
{
	size_t sz;
	Deserialize(f, sz);
	x.reserve(sz);
	for (size_t i = 0; i < sz; ++i)
	{
		TKey key;
		Deserialize(f, key);
		TValue value;
		Deserialize(f, value);
		x[key] = value;
	}
}