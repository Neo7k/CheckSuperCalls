#include "Files.h"
#include "Strings.h"
#include "Config.h"


template<typename Func>
void Walk(CodeType code_type, const Config& config, Func&& functor)
{
	namespace fs = std::filesystem;
	fs::recursive_directory_iterator end;

	for (auto& parse_entity : config.GetParseStructure())
	{
		if (!parse_entity.dir.empty())
		{
			for (fs::recursive_directory_iterator it(parse_entity.dir); it != end; ++it)
			{
				auto& entry = *it;
				bool skip_this_entry = false;
				for (auto& skip : parse_entity.skip)
				{
					if (entry.is_regular_file())
					{
						if (!skip.file.empty())
						{
							auto skip_file_path = parse_entity.dir;
							skip_file_path /= skip.file;
							if (entry.path() == skip_file_path)
							{
								skip_this_entry = true;
								break;
							}
						}
					}
					else if (entry.is_directory())
					{
						if (!skip.dir.empty())
						{
							auto skip_dir_path = parse_entity.dir;
							skip_dir_path /= skip.dir;
							if (entry.path() == skip_dir_path)
							{
								skip_this_entry = true;
								break;
							}
						}
					}
				}

				if (skip_this_entry)
				{
					it.disable_recursion_pending();
					continue;
				}

				if (entry.is_regular_file())
				{
					for (auto& ext : config.GetExt(code_type))
					{
						if (entry.path().extension() == ext)
						{
							functor(entry.path());
						}
					}
				}
			}
		}
		else
		{
			functor(parse_entity.file);
		}
	}
}

void CodeFiles::Collect(const Config& config)
{
	const uint possible_files_count = 16384; // cuz why not

	Walk(CodeType::Header, config, [&](auto& path) 
	{
		headers.emplace_back(path);
	});
	
	Walk(CodeType::Source, config, [&](auto& path) 
	{
		source.emplace_back(path);
	});
}

const FsPaths& CodeFiles::GetHeaders() const
{
	return headers;
}

const FsPaths& CodeFiles::GetSource() const
{
	return source;
}

bool ReadContent(const std::filesystem::path& path, std::string& content)
{
	std::ifstream f(path, std::ios_base::binary);
	if (!f)
		return false;

	f.seekg(0, std::ios::end);
	size_t fsize = (size_t)f.tellg();
	content.resize(fsize);
	f.seekg(0, std::ios::beg);
	f.read(&content[0], fsize);
	NormalizeLineEndings(content);
	return true;
}
