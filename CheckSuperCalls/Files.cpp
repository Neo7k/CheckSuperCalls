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


	bool Match(const std::string& text, const std::string& pattern)
	{
		constexpr int string_begin = -1;
		constexpr int string_end = -2;
		auto&& getc = [](int i, const std::string& s, int string_size)
		{
			if (i <= -1)
				return string_begin;
			if (i >= string_size)
				return string_end;

			return (int)((unsigned char)s[i]);
		};

		auto&& find = [&getc](const std::string& s1, int pos1,
			const std::string& s2, int pos2)
		{
			int i = pos1;
			int j = pos2;
			int i_match = -1;
			int s1_size = (int)s1.size();
			int s2_size = (int)s2.size();
			while (true)
			{
				if (i > s1_size)
					return std::make_pair(-1, -1);

				int ch1 = getc(i, s1, s1_size);
				int ch2 = getc(j, s2, s2_size);

				if (ch2 == '*')
					return std::make_pair(i, j);

				if (ch1 == ch2 ||
					(ch2 == '?' && ch1 != string_begin && ch1 != string_end))
				{
					if (ch1 == string_end)
						return std::make_pair(i, j);

					if (i_match == -1)
						i_match = i;

					i++;
					j++;
				}
				else
				{
					if (j == pos2)
						i++;

					j = pos2;
					if (i_match != -1)
					{
						i = i_match + 1;
						i_match = -1;
					}
				}
			}

			return std::make_pair(-1, -1);
		};

		int i = -1;
		int j = -1;
		int pattern_size = (int)pattern.size();
		while (j <= pattern_size)
		{
			while (getc(j, pattern, pattern_size) == '*')
				j++;

			auto ij = find(text, i, pattern, j);
			if (ij.first == -1)
				return false;

			i = ij.first;
			j = ij.second;
			if (j == pattern.size())
				return true;
		}

		return true;
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
				std::string child(paths[1]);
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
