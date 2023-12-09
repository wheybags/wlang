#pragma once
#include <string>
#include <vector>

bool runProcess(const std::vector<std::string>& args, std::string& output, int32_t& exitCode, void* environmentPtr = nullptr);