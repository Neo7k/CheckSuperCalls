#include <sstream>
#include "Config.h"
#include "Strings.h"
#include "tinyxml2.h"

bool Config::ParseConfig(const std::filesystem::path& path)
{
    using namespace tinyxml2;
    namespace fs = std::filesystem;

    XMLDocument xml_doc;
    if (xml_doc.LoadFile(path.string().c_str()) != tinyxml2::XML_SUCCESS)
        return false;

    auto root = xml_doc.FirstChild();
    if (!root)
        return false;

    auto code_elem = root->FirstChildElement("Code");
    if (!code_elem)
        return false;

    auto headers_elem = code_elem->FirstChildElement("Headers");
    if (!headers_elem)
        return false;

    if (auto ext_c = headers_elem->Attribute("ext"))
    {
        std::istringstream s(ext_c);
        std::string ext;
        while (getline(s, ext, '|'))
        {
            header_ext.push_back(ext);
        }
    }
    else
        return false;

    auto source_elem = code_elem->FirstChildElement("Source");
    if (!source_elem)
        return false;

    if (auto ext_c = source_elem->Attribute("ext"))
    {
        std::istringstream s(ext_c);
        std::string ext;
        while (getline(s, ext, '|'))
        {
            source_ext.push_back(ext);
        }
    }
    else
        return false;


    auto dir_elem = code_elem->FirstChildElement("Dir");
    while (dir_elem)
    {
		fs::path scan_path;
        if (auto path_text = dir_elem->Attribute("path"))
        {
            fs::path dir_path(path_text);
            if (dir_path.is_relative())
            {
                auto full_path = fs::absolute(path).parent_path();
                full_path /= dir_path;
				scan_path = full_path;
            }
            else
				scan_path = path_text;

			if (fs::exists(scan_path))
			{
				auto& p = parse.emplace_back();
				p.dir = scan_path;
			}
			else
			{
				dir_elem = dir_elem->NextSiblingElement("Dir");
				continue;
			}
        }

        auto skip_elem = dir_elem->FirstChildElement("Skip");
        while (skip_elem)
        {
            auto& s = parse.back().skip.emplace_back();
            if (auto skip_dir = skip_elem->Attribute("dir"))
                s.dir = skip_dir;
            else if (auto skip_file = skip_elem->Attribute("file"))
                s.file = skip_file;

            skip_elem = skip_elem->NextSiblingElement("Skip");
        }

        dir_elem = dir_elem->NextSiblingElement("Dir");
    }

    auto file_elem = code_elem->FirstChildElement("File");
    while (file_elem)
    {
		fs::path scan_path;
        if (auto path_text = file_elem->Attribute("path"))
        {
			fs::path file_path(path_text);
			if (file_path.is_relative())
			{
				auto full_path = fs::absolute(path).parent_path();
				full_path /= file_path;
				scan_path = full_path;
			}
			else
				scan_path = path_text;

			if (fs::exists(scan_path))
			{
				auto& p = parse.emplace_back();
				p.file = scan_path;
			}
        }

        file_elem = file_elem->NextSiblingElement("File");
    }

    auto options_elem = root->FirstChildElement("Options");
    if (options_elem)
    {
        auto threads_elem = options_elem->FirstChildElement("Threads");
        if (threads_elem)
            num_threads = threads_elem->IntAttribute("num", 1);
    }

	auto annex_elem = root->FirstChildElement("Annex");
	if (annex_elem)
	{
		auto class_elem = annex_elem->FirstChildElement("Class");
		while (class_elem)
		{
			auto& clazz = annex.emplace_back();
			if (auto name_text = class_elem->Attribute("name"))
			{
				ParseNameWithNamespaceBackwards(name_text + strlen(name_text), clazz.name, clazz.namespase, name_text - 1);
			}
			class_elem = class_elem->NextSiblingElement("Class");
		}
	}


    return true;
}

const string_vector& Config::GetExt(CodeType type) const
{
    return type == CodeType::Header ? header_ext : source_ext;
}

const std::vector<Parse>& Config::GetParseStructure() const
{
    return parse;
}

int Config::GetNumThreads() const
{
    return num_threads;
}
