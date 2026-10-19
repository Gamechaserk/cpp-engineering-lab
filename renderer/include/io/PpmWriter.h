// renderer/include/io/PpmWriter.h
#pragma once
#include <string>
#include "render/Framebuffer.h"

// 契约：写二进制 PPM(P6)。成功返回 true，任何一步失败返回 false。
bool writePpm(const std::string& path, const Framebuffer& fb);