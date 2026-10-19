#include "render/Framebuffer.h"


Framebuffer::Framebuffer(int width, int height)
	:m_width(width), m_height(height), m_pixels(static_cast<std::size_t>(width) * height, Vec3(0.0f, 0.0f, 0.0f)) {}//待补充合法性校验

int Framebuffer::width() const {
	return m_width;
}
int Framebuffer::height() const {
	return m_height;
}
void Framebuffer::set(int x, int y, const Vec3& color) {
	m_pixels[index(x, y)] = color;
}
const Vec3& Framebuffer::at(int x, int y) const {
	return m_pixels[index(x, y)];
}
std::size_t Framebuffer::index(int x, int y) const {
	return static_cast<std::size_t>(y) * m_width + x;
}