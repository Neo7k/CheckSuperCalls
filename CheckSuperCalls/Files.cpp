#include "Files.h"
#include "Strings.h"
#include "Config.h"
#include "Exception.h"

namespace
{
	void UnifySeparators(std::string& path)
	{
		std::ranges::replace(path, '\\', '/');
	}

	struct PathNode
	{
		using Ptr = std::unique_ptr<PathNode>;
		enum Mode
		{
			Add,
			Skip
		};

		Mode mode = Skip;
		std::string path;

		PathNode* parent = nullptr;
		std::vector<Ptr> children;
	};

	PathNode::Ptr ParseFilesConf(const fs::path& path)
	{
		std::ifstream f(path);

		string_vector path_stack;
		int indent = 0;

		auto tree = std::make_unique<PathNode>();
		tree->path = fs::canonical(path).parent_path().make_preferred().string();
		std::replace(tree->path.begin(), tree->path.end(), '\\', '/');
		PathNode* current_node = tree.get();

		while (!f.eof())
		{
			std::string line;
			std::getline(f, line);
			int i = 0;
			for (; i < line.size() && line[i] == '\t'; i++)
			{
			}

			int line_indent = i;
			if (line_indent == line.size() || line[line_indent] == '\n')
				continue; // indented empty line

			if (line_indent - indent > 1)
				throw Exception("Invalid indentation of the path: %s", line.c_str());

			if (line_indent - indent > 0)
				current_node = current_node->children.back().get();
			else
			{
				for (int j = 0; j > line_indent - indent; j--)
					current_node = current_node->parent;
			}

			indent = line_indent;

			PathNode::Mode mode;
			if (line[i] == '+')
			{
				mode = PathNode::Add;
			}
			else if (line[i] == '-')
			{
				mode = PathNode::Skip;
			}
			else
				throw Exception("Expected + or - at the beginning of the path: %s", line.c_str());

			auto new_node = std::make_unique<PathNode>();

			new_node->mode = mode;
			std::replace(line.begin(), line.end(), '\\', '/');
			new_node->path = line.substr(i + 1);
			new_node->parent = current_node;
			current_node->children.push_back(std::move(new_node));
		}

		return tree;
	}


	template <typename T>
	bool MatchImpl(T it_txt, T it_ptn)
	{
		constexpr int no_char = -1;
		auto&& get = [](auto& its_pair)
		{
			if (its_pair.first != its_pair.second)
				return (int)(unsigned char)*its_pair.first;
			return no_char;
		};

		auto&& inc = [](auto& its_pair)
		{
			if (its_pair.first != its_pair.second)
			{
				++its_pair.first;
				return true;
			}
			return false;
		};

		bool wildcart = false;
		while (true)
		{
			if (get(it_ptn) == '*')
			{
				wildcart = true;
				inc(it_ptn);
				continue;
			}

			if (get(it_txt) == get(it_ptn) ||
				(get(it_ptn) == '?' && get(it_txt) != no_char))
			{
				if (get(it_txt) == no_char && get(it_ptn) == no_char)
					return true;

				inc(it_txt);
				inc(it_ptn);
				wildcart = false;
				continue;
			}

			if (wildcart)
			{
				if (!inc(it_txt))
					return false;

				continue;
			}

			return false;
		}
	}

	bool Match(const std::string& text, const std::string& pattern)
	{
		return	MatchImpl(std::make_pair(text.begin(), text.end()), std::make_pair(pattern.begin(), pattern.end())) ||
			MatchImpl(std::make_pair(text.rbegin(), text.rend()), std::make_pair(pattern.rbegin(), pattern.rend()));
	}

	template<typename Func>
	void WalkImpl(const fs::path& path, PathNode* node, Func&& func)
	{
		if (node && node->mode == PathNode::Skip && node->children.empty())
			return;

		func(path);

		if (fs::is_directory(path))
		{
			fs::directory_iterator end;
			for (fs::directory_iterator it(path); it != end; ++it)
			{
				auto child_path = it->path();
				bool rule_applied = false;
				if (node)
				{
					for (auto& child : node->children)
					{
						if (Match(child_path.filename().string(), child->path))
						{
							WalkImpl(child_path, child.get(), std::forward<Func>(func));
							rule_applied = true;
							break;
						}
					}
				}

				if (!rule_applied)
				{
					if (!node || node->mode == PathNode::Add)
						WalkImpl(child_path, nullptr, std::forward<Func>(func));
				}
			}
		}

	}

	template<typename Func>
	void Walk(PathNode* path_tree, Func&& func)
	{
		for (auto& path_node : path_tree->children)
		{
			fs::path path(path_node->path);
			if (!path.is_absolute())
			{
				fs::path root(path_tree->path);
				path = root / path;
			}

			WalkImpl(path, path_node.get(), std::forward<Func>(func));
		}
	}

	std::vector<std::string_view> Tokenize(std::string_view str, char token)
	{
		std::vector<std::string_view> result;
		result.reserve(std::ranges::count(str, token));
		size_t pos = 0;
		while (true)
		{
			size_t tok = str.find(token, pos);
			if (tok != std::string_view::npos)
			{
				result.push_back(str.substr(pos, tok - pos));
				pos = tok + 1;
			}
			else
			{
				result.push_back(str.substr(pos));
				break;
			}
		}

		return result;
	}

	void UnrollTree(PathNode* node)
	{
		if (fs::path(node->path).is_relative())
		{
			auto paths = Tokenize(node->path, '/');
			if (paths.size() > 1)
			{
				std::string root(paths[0]);
				std::string child(node->path.substr(root.length() + 1));
				PathNode::Mode old_mode = node->mode;

				node->path = root;
				node->mode = node->parent ? node->parent->mode : PathNode::Skip;
				auto new_node = std::make_unique<PathNode>();
				new_node->path = child;
				new_node->mode = old_mode;
				new_node->parent = node;
				new_node->children = std::move(node->children);
				for (auto& child : new_node->children)
					child->parent = new_node.get();

				node->children = decltype(node->children){};
				node->children.push_back(std::move(new_node));
			}
		}

		for (auto& child : node->children)
		{
			UnrollTree(child.get());
		}
	}
}


CodeFiles::CodeFiles(const fs::path& path, const Config& config) 
{
	// Now this can be either a path to the directory with the code
	// or a path to the file with the directories structure description
	if (!fs::exists(path))
		throw Exception("Path %s does not exist", path.c_str());

	PathNode::Ptr path_tree;
	if (fs::is_directory(path))
	{
		path_tree.reset(new PathNode);
		auto path_str = fs::canonical(path).string();
		UnifySeparators(path_str);
		path_tree->path = path_str;

		auto child = new PathNode;
		child->path = path_str;
		child->mode = PathNode::Add;
		path_tree->children.emplace_back(child);
	}
	else
	{
		path_tree = ParseFilesConf(path);
		UnrollTree(path_tree.get());
	}

	Walk(path_tree.get(), [&config, this](auto& path) 
		{
			auto source_ext = config.GetExt(CodeType::Header);
			auto header_ext = config.GetExt(CodeType::Source);
			if (!fs::is_directory(path))
			{
				auto ext = path.extension();
				auto it_header = std::ranges::find(header_ext, ext);
				if (it_header != header_ext.end())
					source.push_back(path);
				else
				{
					auto it_source = std::ranges::find(source_ext, ext);
					if (it_source != source_ext.end())
					{
						headers.push_back(path);
					}
				}
			}
		});

	std::ofstream f("files.txt");
	for (auto& s : headers)
		f << fs::canonical(s) << std::endl;
	for (auto& s : source)
		f << fs::canonical(s) << std::endl;
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
