#include "Config.h"
#include "Strings.h"
#include "Exception.h"

Config::Config(const fs::path& path)
{
	if (!fs::exists(path))
		throw Exception(std::format("Path {} doesn't exit", path.string()));

	std::ifstream f(path);
	while (!f.eof())
	{
		std::string line;
		std::getline(f, line);
		
		if (line.empty())
			continue;

		if (line[0] == '#')
			continue;

		auto tokens = Tokenize(line, '=');
		if (tokens.size() < 2)
			continue;

		auto&& read_string = [](const auto& token)
		{
			auto filtered_token = token | std::views::filter([](char c) {return c != '"'; });
			std::string str(std::begin(filtered_token), std::end(filtered_token));
			return str;
		};

		auto&& read_extensions = [&read_string](auto& exts, const auto& token)
		{
			auto str = read_string(token);
			auto extensions = Tokenize(str, '|');
			exts.clear();
			for (auto& ext : extensions)
				exts.emplace_back(ext);
		};

		if (tokens[0] == "verbose")
			std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), verbosity);
		else if (tokens[0] == "headers")
			read_extensions(header_ext, tokens[1]);
		else if (tokens[0] == "source")
			read_extensions(source_ext, tokens[1]);
		else if (tokens[0] == "call_super_attribute")
			call_super_keyword = read_string(tokens[1]);
		else if (tokens[0] == "skip_super_attribute")
			skip_super_keyword = read_string(tokens[1]);
	}
}

const std::vector<fs::path>& Config::GetExt(CodeType type) const
{
	return type == CodeType::Header ? header_ext : source_ext;
}

int Config::GetVerbosity() const
{
	return verbosity;
}

const char* Config::GetCallSuperKeyword() const
{
	return call_super_keyword.c_str();
}

const char* Config::GetSkipSuperKeyword() const
{
	return skip_super_keyword.c_str();
}