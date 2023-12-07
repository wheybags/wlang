#pragma once
#include <filesystem>

namespace fs = std::filesystem;

fs::path getPathToThisExecutable();
FILE* fopen(const fs::path& path, const std::string& mode);
[[nodiscard]] bool readWholeFileAsString(const std::filesystem::path& path, std::string& string);
[[nodiscard]] bool overwriteFileWithString(const fs::path& path, std::string_view string);
