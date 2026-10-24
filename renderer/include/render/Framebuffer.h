// renderer/include/render/Framebuffer.h
#pragma once
#include <vector>
#include "math/Vec3.h"

// 约定：Framebuffer(0,0) = 图像【左上角】；行号向下增大（与 PPM 一致）
//       数学库的 y 轴【向上】→ U01 视口变换时需做一次 y 翻转
//
// ★ U02 起本类同时持有【颜色缓冲】与【深度缓冲】（每像素一个"到目前为止最近的深度"）。
//   两者尺寸相同、索引方式相同；构造函数把深度全部初始化成 FAR_DEPTH（= 最远）。
//   ★ 这条不变式正是 U01 的用例仍然成立的原因：初始即最远 ⇒ 第一次绘制时每个被覆盖的
//     像素都能通过深度测试 ⇒ "返回的写入像素数" == "三角形覆盖的像素数"（Pick 定理那 3281 条）。
class Framebuffer {
public:
    // 深度的"最远"初值：任何真实片元的 NDC z 都比它小（NDC z ∈ [-1,1]，【小 = 近】）
    // 用 FLT_MAX 而不是 +inf：语义等价，且避免 inf 参与比较时的意外
    static constexpr float FAR_DEPTH = 3.402823466e+38f;

    Framebuffer(int width, int height);

    int width() const;
    int height() const;

    // 清屏：颜色全部置为 color，深度全部重置为 FAR_DEPTH
    void clear(const Vec3& color);

    // 只写颜色、不参与深度测试（清屏 / 2D 用途；U02 的三维绘制请用 setDepth）
    void set(int x, int y, const Vec3& color);

    // ★ U02：带深度的写入 —— 调用方【必须】已经用 depthAt() 做过深度测试
    void setDepth(int x, int y, float z, const Vec3& color);

    const Vec3& at(int x, int y) const;

    // ★ U02：读该像素当前记录着的最近深度（刚构造 / clear 之后 = FAR_DEPTH）
    float depthAt(int x, int y) const;

private:
    std::size_t index(int x, int y) const;
    int m_width, m_height;
    std::vector<Vec3> m_pixels;
    std::vector<float> m_depth;   // ★ U02 新增：与 m_pixels 同尺寸、同索引
};