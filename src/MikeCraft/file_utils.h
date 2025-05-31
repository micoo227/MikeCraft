#pragma once

#include <string>

std::string getExecutableDir();
void        ensureWorldFolderExists();
std::string getRegionFilePath(const std::string& regionFileName);