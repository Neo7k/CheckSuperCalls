#include "Annex.h"
#include "Strings.h"
#include "tinyxml2.h"

bool Annex::Parse(const fs::path& path)
{
	if (path.empty())
		return false;

	using namespace tinyxml2;

	XMLDocument xml_doc;
	if (xml_doc.LoadFile(path.string().c_str()) != tinyxml2::XML_SUCCESS)
		return false;

	auto root = xml_doc.FirstChild();
	if (!root)
		return false;

	auto class_elem = root->FirstChildElement("Class");
	while (class_elem)
	{
		auto& clazz = classes.emplace_back();
		if (auto classname = class_elem->Attribute("name"))
			ParseNameWithNamespaceBackwards(classname + strlen(classname) - 1, clazz.name, clazz.namespase, classname - 1);

		auto func_elem = class_elem->FirstChildElement("CallSuper");
		while (func_elem)
		{
			if (auto funcname = func_elem->Attribute("name"))
				clazz.functions.emplace_back(funcname);
			
			func_elem = func_elem->NextSiblingElement("CallSuper");
		}
		class_elem = class_elem->NextSiblingElement("Class");
	}

	return true;
}