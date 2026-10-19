#include<iostream>
#include <direct.h>      // _mkdir：C++14 没有 std::filesystem
#include <cstdio>
#include "render/Framebuffer.h"
#include "io/PpmWriter.h"

int main() {
    _mkdir("output");                       // 目录已存在时会返回 -1，忽略即可
    const int W = 256, H = 256;
    Framebuffer fb(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            fb.set(x, y, Vec3(static_cast<float>(x) / (W - 1),
                static_cast<float>(y) / (H - 1),
                0.5f));
    const char* path = "output/gradient.ppm";
    if (!writePpm(path, fb)) { std::printf("write failed: %s\n", path); return 1; }
    std::printf("wrote %s (%dx%d)\n", path, W, H);
    return 0;
}