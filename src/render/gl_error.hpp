#pragma once
#include <string>
namespace Glasscope {
void clearGlErrors();
bool checkGlError(const char* operation, std::string& error);
}
