#pragma once
#include <functional>
#include <filesystem>
#include "Config.h"

void Walk(CodeType code_type, const Config& config, const std::function<void(const std::filesystem::path& path)>& functor);
bool ReadContent(const std::filesystem::path& path, std::string& content);
