#pragma once

// ============================================================================
//  U0 前置 · 顶点流水线前段：世界坐标 -> clip space -> NDC
//
//   这一层【不碰 Rasterizer】
//     rasterizeTriangle / toScreenX / toScreenY / Vertex3D 全部保持原样
//     ⇒ 现有 70 个用例的契约一个字不改（这是 11-04 不该付的代价）
//
//   管线位置：
//      世界坐标 --mvp--> clip space --[裁剪]--> --/w--> NDC --> 交给 rasterizeTriangle
//                        ↑ 这一层负责                      ↑ 现有的
//
//   为什么要单独一层：Rasterizer 的契约是"NDC 进、像素出"，已由 70 用例钉住；
//    把 MVP 塞进去会同时改掉它的输入语义（§十 我要报的那条冲突）
// ============================================================================

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "render/Rasterizer.h"   // 复用 Vertex3D（NDC 顶点）

// 单个顶点：世界坐标 + MVP -> clip space 齐次坐标
//  不做透视除法（除法是单独的工序，也是题 2 的考点）
Vec4 worldToClip(const Mat4& mvp, const Vec3& worldPoint);

// 三角形：世界坐标 -> NDC
//   返回 true  = 可以交给 rasterizeTriangle 画
//   返回 false = 整个三角形丢弃
//
// ★ 11-04 闸门版的裁剪规则（先求正确，不先求完整）：
//     三个顶点的 w 全部 > 0  ⇒ 做透视除法，返回 true
//     只要有一个 w <= 0      ⇒ 返回 false（整个丢弃）
//   ⚠️ 这是"最小可用裁剪"，会有洞：相机贴着物体时，跨近平面的三角形会整片消失。
//      完整的多边形裁剪（把三角形切成多边形、求交点）留到 11-07 之后。
//
// ★ 实现提示：
//     clip = mvp.transformPoint(worldPoint)          // Vec4
//     if (clip.w <= 0) 丢弃
//     ndc  = Vec3(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w)
//     na   = Vertex3D{ndc.x, ndc.y, ndc.z}
bool worldTriangleToNdc(const Mat4& mvp,
                        const Vec3& wa, const Vec3& wb, const Vec3& wc,
                        Vertex3D& na, Vertex3D& nb, Vertex3D& nc);