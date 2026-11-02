#include "render/Shading.h"

// ============================================================================
//  U03 · Shading.cpp  —— 【学员实现文件（教练只放骨架，函数体返回 0）】
//
//  ★ 现在 ctest 会是红的：新增的 7 条 Shading.* 用例全失败 —— 这就是你的任务。
//  ★ 把下面两个函数按 Shading.h 的注释实现，把 7 条跑绿即可。
// ============================================================================

namespace render {

Vec3 blinnPhong(const Vec3& P, const Vec3& N, const Vec3& V,
                const Light& L, const Material& M) {
    (void)P; (void)N; (void)V; (void)L; (void)M;
    // TODO(学员)：ambient = ka * albedo
    //             Ldir    = (L.position - P).normalized()
    //             diffuse = max(0, N·Ldir) * albedo
    //             H       = (Ldir + V).normalized()
    //             specular= max(0, N·H)^shininess * ks
    //             三项乘 L.color * L.intensity（ambient 按约定不乘光强）
    //             逐通道 clamp 到 [0, 1]
    return Vec3(0.0f, 0.0f, 0.0f);
}

float signedArea2D(float ax, float ay,
                   float bx, float by,
                   float cx, float cy) {
    (void)ax; (void)ay; (void)bx; (void)by; (void)cx; (void)cy;
    // TODO(学员)：2 倍有向面积 = (bx-ax)*(cy-ay) - (by-ay)*(cx-ax)
    //             ★ 务必与 Rasterizer.cpp 里的边函数同一套符号约定
    return 0.0f;
}

} // namespace render