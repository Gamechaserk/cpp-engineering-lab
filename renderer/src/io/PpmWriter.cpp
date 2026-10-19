#include "io/PpmWriter.h"        // 先包含自己的头
#include <fstream>
#include <iostream>
#include <vector>

namespace {
    unsigned char toByte(float v) {
        if (v <= 0.0f) return 0;
        if (v >= 1.0f) return 255;
        return static_cast<unsigned char>(v * 255.0f + 0.5f);
    }
}

bool writePpm(const std::string& path, const Framebuffer& fb) {   // 这一行和头文件里逐字一致
    // 真正干活的代码：binary 打开、写文件头、写像素、检查 out.good()
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << fb.width() << " " << fb.height() << "\n255\n";

    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            const Vec3& c = fb.at(x, y);
            out.put(static_cast<char>(toByte(c.x)));
            out.put(static_cast<char>(toByte(c.y)));
            out.put(static_cast<char>(toByte(c.z)));
        }
    }

    out.close();          // 显式关闭：把缓冲刷到磁盘，让错误可以被观察到
    return out.good();    // 现在这个判断才真正可信
}