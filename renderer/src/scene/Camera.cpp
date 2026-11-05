#include "scene/Camera.h"

// ============================================================================
//  U0 前置 · Camera.cpp —— 【学员实现文件（教练只放骨架）】
//
//   四个方法全部是一行：包一层已经写好的 Mat4 函数
//    viewMatrix()      -> Mat4::lookAt(position, target, up)
//    projMatrix()      -> Mat4::perspective(fovYRad, aspect, nearZ, farZ)
//    mvpMatrix(model)  -> projMatrix() * viewMatrix() * model     ←  右乘先做
//    projectToClip()   -> mvpMatrix(model).transformPoint(worldPoint)
//                          不做透视除法
//
//   只有构造函数要算一下：45° 转弧度
// ============================================================================

Camera::Camera()
    : position(0.0f, 0.0f, 5.0f),
      target(0.0f, 0.0f, 0.0f),
      up(0.0f, 1.0f, 0.0f),
      fovYRad(45.0f * 3.14159265358979323846f / 180.0f),
      aspect(1.0f),
      nearZ(0.1f),
      farZ(100.0f) {}

Mat4 Camera::viewMatrix() const {
    return Mat4::lookAt(position, target, up);
}

Mat4 Camera::projMatrix() const {
    return Mat4::perspective(fovYRad, aspect, nearZ, farZ);
}

Mat4 Camera::mvpMatrix(const Mat4& model) const {
    return projMatrix() * viewMatrix() * model;
}

Vec4 Camera::projectToClip(const Vec3& worldPoint, const Mat4& model) const {
    return mvpMatrix(model).transformPoint(worldPoint);
}