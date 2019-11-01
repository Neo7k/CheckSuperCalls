#include "Code.h"
#include "Strings.h"
#include "Files.h"
#include "Exception.h"
#include "Result.h"
#include "Annex.h"

Code::~Code()
{
	for (auto& clazz : classes)
		delete clazz;

	classes.clear();
}

void Code::ParseLookup(const std::string& content, const fs::path& path)
{
	std::array<uint, 2> cl_st = { EKeywords::Class, EKeywords::Struct };
	std::array<uint, 4> cur_scol = { EKeywords::Cur, EKeywords::Colon, EKeywords::SColon, EKeywords::Paren };

	size_t pos = 0;
	auto find_res = find_first(content, pos, cl_st);
	while (find_res)
	{
		pos = find_res.pos_end;

		auto find_res_class = find_first(content, pos, cur_scol);
		if (find_res_class.key == EKeywords::Colon || find_res_class.key == EKeywords::Cur)
		{
			if (content[find_res_class.pos_end] != ':')
			{
				auto classname = ParseClassNameBackwards(&content[find_res_class.pos-1], &content[pos]);
				if (!classname.empty())
				{
					std::unique_lock lock(classdef_lookup_mutex);
					classdef_lookup[classname].insert(path);
				}
			}
			else
				pos = find_res_class.pos_end + 1;
		}
		pos = find_res_class.pos_end;
		find_res = find_first(content, pos, cl_st);
	}
}


void Code::ParseHeaderForBaseClasses(const std::string& content)
{
	std::array<uint, 1> sup = { EKeywords::CallSuper };
	auto find_res = find_first(content, 0, sup);
	if (!find_res)
		return;

	size_t pos = 0;
	NamespaseStack stack;
	int attri_stack = 0;
	std::array<uint, 10> search_keys =
	{
		EKeywords::Namespace,
		EKeywords::Class,
		EKeywords::Struct,
		EKeywords::CallSuper,
		EKeywords::Cur,
		EKeywords::Ly,
		EKeywords::ComStart,
		EKeywords::SCom,
		EKeywords::AttriBra,
		EKeywords::AttriKet
	};

	find_res = find_first(content, pos, search_keys);
	while (find_res)
	{
		pos = find_res.pos_end;
		if (SkipComments(find_res.key, content, pos))
		{
			find_res = find_first(content, pos, search_keys);
			continue;
		}

		if (find_res.key == EKeywords::Namespace)
			ParseNamespace(content, pos, stack);
		else if (find_res.key == EKeywords::Struct || find_res.key == EKeywords::Class)
		{
			std::array<uint, 4> scl_cur_ket = { EKeywords::SColon, EKeywords::Colon, EKeywords::Cur, EKeywords::Ket };
			auto fres = find_first(content, pos, scl_cur_ket);
			if (fres.key == EKeywords::Colon)
			{
				if (content[fres.pos_end] != ':')
				{
					stack.Push(ParseClassNameBackwards(&content[fres.pos-1], &content[pos]));
					pos = fres.pos_end;
					std::array<uint, 1> cur = { EKeywords::Cur };
					fres = find_first(content, pos, cur);
					if (fres.key == EKeywords::Cur)
						pos = fres.pos_end;
				}
				else
					pos = fres.pos_end + 1;
			}
			else if (fres.key == EKeywords::Cur)
			{
				stack.Push(ParseClassNameBackwards(&content[fres.pos - 1], &content[pos]));
				pos = fres.pos_end;
			}
			// else -> Forward declaration or inside of a template
		}
		else if (find_res.key == EKeywords::Cur)
			stack.Push();
		else if (find_res.key == EKeywords::Ly)
			stack.Pop();
		else if (find_res.key == EKeywords::CallSuper && attri_stack == 1)
		{
			std::array<uint, 1> atket = { EKeywords::AttriKet };
			auto fres = find_first(content, pos, atket);
			if (fres.key == EKeywords::AttriKet)
			{
				--attri_stack;
				pos = fres.pos_end;
				std::array<uint, 1> par = { EKeywords::Paren };
				fres = find_first(content, pos, par);
				if (fres.key == EKeywords::Paren)
				{
					auto funcname = GetName(&content[fres.pos - 1], true);
					if (stack.namespase.empty())
						throw ParseException(fres.pos, "Expected the function '%s' to be inside of the class", funcname.c_str());

					auto classname = stack.namespase.back();
					if (classname.empty())
						throw ParseException(fres.pos, "Empty class name found");

					auto clazz = GetClass(classname, stack.namespase_trunk, string_vector(), true);
					if (!clazz)
						clazz = CreateClass(classname, stack.namespase_trunk);

					// Leaving the class without mutex lock
					// That should not call the race condition,
					// since a single class definition can't be split into different header files
					// which the threads are working on
					clazz->functions.push_back(funcname);
					{
						std::unique_lock lock(functions_call_super_mutex);
						functions_call_super.push_back(funcname);
					}
					pos = fres.pos_end;
				}
				else
					throw ParseException(pos, "Expected a '(' in function declaration");
			}
		}
		else if (find_res.key == EKeywords::AttriBra)
		{
			++attri_stack;
			if (attri_stack > 1)
				throw ParseException(pos, "Encountered nested [[ ]]");
		}
		else if (find_res.key == EKeywords::AttriKet)
			--attri_stack;

		find_res = find_first(content, pos, search_keys);
	}
}

void Code::ApplyAnnex(const Annex& annex)
{
	for (auto& aclazz : annex.classes)
	{
		auto clazz = GetClass(aclazz.name, aclazz.namespase, string_vector(), false);
		if (!clazz)
			clazz = CreateClass(aclazz.name, aclazz.namespase);

		clazz->functions = aclazz.functions;
		for (auto& func : aclazz.functions)
			functions_call_super.push_back(func);
	}
}

void Code::ParseHeader(const fs::path& path, const std::string& content, bool skip_if_no_target_funcs/* = true*/)
{
	if (skip_if_no_target_funcs)
	{
		std::array<uint, 0> none;
		auto find_res = find_first(content, 0, none, functions_call_super);
		if (!find_res)
			return;
	}

	size_t pos = 0;
	NamespaseStack stack;
	std::array<uint, 7> search_keys =
	{
		EKeywords::Namespace,
		EKeywords::Class,
		EKeywords::Struct,
		EKeywords::Cur,
		EKeywords::Ly,
		EKeywords::ComStart,
		EKeywords::SCom
	};

	auto find_res = find_first(content, pos, search_keys);
	while (find_res)
	{
		pos = find_res.pos_end;
		if (SkipComments(find_res.key, content, pos))
		{
			find_res = find_first(content, pos, search_keys);
			continue;
		}

		if (find_res.key == EKeywords::Namespace)
			ParseNamespace(content, pos, stack);
		else if (find_res.key == EKeywords::Struct || find_res.key == EKeywords::Class)
		{
			std::array<uint, 4> scl_cur_ket = { EKeywords::SColon, EKeywords::Colon, EKeywords::Cur, EKeywords::Ket };
			auto fres = find_first(content, pos, scl_cur_ket);
			if (fres.key == EKeywords::Colon)
			{
				if (content[fres.pos_end] != ':')
				{
					auto classname = ParseClassNameBackwards(&content[fres.pos - 1], &content[pos]);
					if (classname.empty())
						throw ParseException(fres.pos, "Empty class name found");

					string_vector pos_ns;
					auto clazz = GetClass(classname, stack.namespase, string_vector(), true);
					if (!clazz)
						clazz = CreateClass(classname, stack.namespase);

					pos = fres.pos_end;
					std::array<uint, 1> cur = { EKeywords::Cur };
					fres = find_first(content, pos, cur);
					if (fres.key == EKeywords::Cur)
					{
						std::vector<SuperClassName> inheritance;
						ParseInheritance(&content[pos], &content[fres.pos], inheritance);
						for (auto& parent : inheritance)
						{
							auto& parent_classname = parent.name;
							auto parent_clazz = GetClass(parent_classname, parent.namespase, stack.namespase, true);
							if (!parent_clazz)
							{
								auto it = classdef_lookup.find(parent_classname);
								if (it != classdef_lookup.end())
								{
									auto& parse_paths = it->second;
									for (auto& parent_path : parse_paths)
									{
										if (parent_path == path)
											continue;

										std::string file_contents;
										if (ReadContent(parent_path, file_contents))
											ParseHeader(parent_path, file_contents, false);

									}
								}

								parent_clazz = GetClass(parent_classname, parent.namespase, stack.namespase, true);
								if (!parent_clazz)
									parent_clazz = CreateClass(parent_classname, string_vector());
							}

							clazz->super_classes.push_back(parent_clazz);
							parent_clazz->derived_classes.push_back(clazz);
						}
						pos = fres.pos_end;
						stack.Push(std::move(classname));
					}
				}
				else
					pos = fres.pos_end + 1;
			}
			else if (fres.key == EKeywords::Cur)
			{
				auto classname = TrimSpaces(&content[pos], &content[fres.pos]);

				if (!classname.empty())
				{
					auto clazz = GetClass(classname, stack.namespase, string_vector(), true);
					if (!clazz)
						clazz = CreateClass(classname, stack.namespase);

					pos = fres.pos_end;
					stack.Push(std::move(classname));
				}
			}
			// else -> Forward declaration or inside of a template
		}
		else if (find_res.key == EKeywords::Cur)
			stack.Push();
		else if (find_res.key == EKeywords::Ly)
			stack.Pop();

		find_res = find_first(content, pos, search_keys);
	}
}

const Class* Code::FindFuncInSuper(const std::string func_name, const Class* clazz) const
{
	if (!clazz)
		return nullptr;

	for (auto& function : clazz->functions)
		if (function == func_name)
			return clazz;

	for (auto parent : clazz->super_classes)
		if (auto parent_clazz = FindFuncInSuper(func_name, parent))
			return parent_clazz;

	return nullptr;
}

void Code::ParseNamespace(const std::string& content, size_t& pos, NamespaseStack& ns)
{
	std::array<uint, 2> cl_cur = { EKeywords::SColon, EKeywords::Cur };
	auto fres = find_first(content, pos, cl_cur); // Looking for 'namespace X {'
	if (fres.key == EKeywords::Cur)
	{
		ns.Push(std::move(TrimSpaces(&content[pos], &content[fres.pos])));
		pos = fres.pos_end;
	}
	// else 'using namespace;' or 'namespace X = Y;'
}

bool Code::IsComment(const std::string& content, size_t pos)
{
	std::array<uint, 2> scom{ EKeywords::SCom, EKeywords::EOL };
	auto fres = find_first_reverse(content, pos, scom);
	return fres.key == EKeywords::SCom;
}

void Code::SkipMultilineComment(const std::string& content, size_t& pos)
{
	std::array<uint, 2> scom{ EKeywords::SCom, EKeywords::EOL };
	auto fres = find_first_reverse(content, pos, scom);
	if (fres.key != EKeywords::SCom)
	{
		std::array<uint, 1> comend{ EKeywords::ComEnd };
		auto fres = find_first(content, pos, comend);
		if (fres)
			pos = fres.pos;
		else
			throw ParseException(pos, "Can't find the end of the multiline comment");
	}
}

bool Code::SkipComments(EKeywords::TYPE key, const std::string& content, size_t& pos)
{
	if (key == EKeywords::SCom)
	{
		std::array<uint, 1> eol{ EKeywords::EOL };
		auto fres = find_first(content, pos, eol);
		if (fres)
		{
			pos = fres.pos_end;
			return true;
		}
		else
			throw ParseException(pos, "Can't find EOL after a //");
	}
	else if (key == EKeywords::ComStart)
	{
		SkipMultilineComment(content, pos);
		return true;
	}
	return false;
}

void Code::ParseCpp(int thread_id, const fs::path& path, const std::string& content, Result& result)
{
	std::array<uint, 2> com{EKeywords::ComStart, EKeywords::SCom};
	size_t pos = 0;
	auto find_res = find_first(content, pos, com, functions_call_super);
	while (find_res)
	{
		if (find_res.key == EKeywords::ComStart || find_res.key == EKeywords::SCom)
		{
			pos = find_res.pos_end;
			if (SkipComments(find_res.key, content, pos))
			{
				find_res = find_first(content, pos, com, functions_call_super);
				continue;
			}
		}

		pos = find_res.pos;
		std::array<uint, 2> cur_scol = { EKeywords::Cur, EKeywords::SColon };
		auto func_decl_find_res = find_first(content, pos, cur_scol);
		if (func_decl_find_res.key == EKeywords::Cur)
		{
			std::string name;
			string_vector namespase;
			auto& func_name = functions_call_super[find_res.extra_index];
			ParseNameWithNamespaceBackwards(&content[find_res.pos - 3], name, namespase); // -3 to skip '::'
			auto clazz = GetClass(name, namespase, string_vector(), false);
			if (clazz)
			{
				auto super_clazz = FindFuncInSuper(func_name, clazz);
				if (super_clazz && super_clazz != clazz)
				{
					std::array<uint, 4> curly = { EKeywords::Cur, EKeywords::Ly, EKeywords::ComStart, EKeywords::SCom };
					string_vector funcname_search{ func_name };
					pos = func_decl_find_res.pos_end;
					auto func_find_res = find_first(content, pos, curly, funcname_search);
					int stack = 1;
					bool super_called = false;
					while (stack > 0)
					{
						if (!func_find_res)
							throw ParseException(pos, "Can't find the end of the function '%s'", func_name.c_str());

						if (SkipComments(func_find_res.key, content, pos))
						{
							func_find_res = find_first(content, pos, curly, funcname_search);
							continue;
						}

						if (func_find_res.key == EKeywords::Cur)
							stack++;
						else if (func_find_res.key == EKeywords::Ly)
							stack--;
						else
							super_called = true;

						pos = func_find_res.pos_end;
						func_find_res = find_first(content, pos, curly, funcname_search);
					}

					if (!super_called)
						result.AddIssue(new Issue{ path, GetLineIndex(content, find_res.pos), DecorateWithNamespace(func_name, name, namespase) }, thread_id);
				}
			}
		}

		pos = func_decl_find_res.pos;
		find_res = find_first(content, pos, com, functions_call_super);
	}
}

size_t Code::GetClassLookupSize() const
{
	return classdef_lookup.size();
}

size_t Code::GetClassesCount() const
{
	return classes.size();
}

size_t Code::GetCSFunctionsCount() const
{
	return functions_call_super.size();
}

Class* Code::GetClass(const std::string& name, const string_vector& namespase, const string_vector& possible_namespase, bool thread_lock) const
{
	if (name.empty())
		return nullptr;

	std::unique_lock lock(classes_mutex, std::defer_lock);
	if (thread_lock)
		lock.lock();

	for (auto& clazz : classes)
	{
		if (clazz->name == name)
		{
			bool ns_match = true;
			auto in = namespase.rbegin();
			auto inp = possible_namespase.rbegin();
			auto inc = clazz->namespase.rbegin();
			bool c_p = false;

			while (true)
			{
				if (!c_p)
				{
					if (in == namespase.rend() &&
						inc == clazz->namespase.rend())
						break;

					if (in == namespase.rend())
					{
						c_p = true;
						continue;
					}

					if (inc == clazz->namespase.rend())
					{
						ns_match = false;
						break;
					}

					if (*in == *inc)
					{
						if (inp != possible_namespase.rend() && *in == *inp)
							inp++;

						in++;
						inc++;
					}
					else
					{
						ns_match = false;
						break;
					}
				}
				else
				{
					if (inc == clazz->namespase.rend())
						break;

					if (inp == possible_namespase.rend())
					{
						ns_match = false;
						break;
					}

					if (*inp == *inc)
					{
						inp++;
						inc++;
					}
					else
					{
						inp++;
					}
				}
			}

			if (ns_match)
				return clazz;
		}
	}

	return nullptr;
}

Class* Code::CreateClass(const std::string& name, const string_vector& namespase)
{
	if (name.empty())
		return nullptr;

	auto clazz = new Class;
	clazz->name = name;
	clazz->namespase = namespase;

	std::unique_lock lock(classes_mutex);
	classes.push_back(clazz);

	return clazz;
}

void Code::NamespaseStack::Push()
{
	++level;
}

void Code::NamespaseStack::Push(std::string&& ns)
{
	++level;
	if (!namespase.empty())
		namespase_trunk.push_back(namespase.back());

	namespase.push_back(ns);
	ns_levels.push_back(level);
}

void Code::NamespaseStack::Pop()
{
	if (!ns_levels.empty() && ns_levels.back() == level)
	{
		namespase.pop_back();
		ns_levels.pop_back();
		if (!namespase_trunk.empty())
			namespase_trunk.pop_back();
	}
	--level;
}