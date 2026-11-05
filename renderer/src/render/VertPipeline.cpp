#include "render/VertPipeline.h"

// ============================================================================
//  U0 前置 · VertPipeline.cpp —— 【学员实现文件（教练只放骨架）】
//
//  ★ 两个函数都只有十几行，但它们是 11-04 闸门的核心：
//    矩阵再对，只要这里缺了 "/w"，立方体就画不出来
//
//  ★ 三条最容易错的地方：
//     ① 忘了"先判 w > 0 再除"  ⇒ 除出 inf / 负号，画面莫名其妙炸掉
//     ② 把 worldTriangleToNdc 的返回值忽略了 ⇒ 相机后面的三角形也被画
//     ③ NDC 的 y 是【上正】，而屏幕 y 是【下正】 —— 这个翻转由 toScreenY 负责，
//        这一层【不要】再翻一次（翻两次等于没翻）
// ============================================================================

Vec4 worldToClip(const Mat4& mvp, const Vec3& worldPoint) {
    return mvp.transformPoint(worldPoint);
}

bool worldTriangleToNdc(const Mat4& mvp,const Vec3& wa, const Vec3& wb, const Vec3& wc,
                        Vertex3D& na, Vertex3D& nb, Vertex3D& nc) {
    Vec4 a = worldToClip(mvp, wa);
    Vec4 b = worldToClip(mvp, wb);
    Vec4 c = worldToClip(mvp, wc);
    if (a.w <= 0 || b.w <= 0 || c.w <= 0) return false;
    na.x = a.x / a.w, na.y = a.y / a.w, na.z = a.z / a.w;
    nb.x = b.x / b.w, nb.y = b.y / b.w, nb.z = b.z / b.w;
    nc.x = c.x / c.w, nc.y = c.y / c.w, nc.z = c.z / c.w;
    return true;
}