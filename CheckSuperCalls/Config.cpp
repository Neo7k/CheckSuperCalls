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
		
		auto tokens = Tokenize(line, '=');
		if (tokens.size() < 2)
			continue;

		auto&& read_extensions = [](auto& exts, const auto& token)
		{
			auto filtered_token = token | std::views::filter([](char c) {return c != '"'; });
			std::string str(std::begin(filtered_token), std::end(filtered_token));
			auto extensions = Tokenize(str, '|');
			exts.clear();
			for (auto& ext : extensions)
				exts.emplace_back(ext);
		};

		if (tokens[0] == "verbose")
			std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), verbosity);
		else if (tokens[0] == "headers")
			read_extensions(header_ext, tokens[1]);
		else if (tokens[1] == "source")
			read_extensions(source_ext, tokens[1]);
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