#include "render/Framebuffer.h"


Framebuffer::Framebuffer(int width, int height)
	:m_width(width), m_height(height),m_depth(static_cast<std::size_t>(width)* height, FAR_DEPTH),
	m_pixels(static_cast<std::size_t>(width) * height, Vec3(0.0f, 0.0f, 0.0f)){}//待补充合法性校验

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

void Framebuffer::clear(const Vec3& color) {
	for (std::size_t i = 0; i < m_pixels.size();i++) {
		m_pixels[i] = color;
		m_depth[i] = FAR_DEPTH;
	}
}
void Framebuffer::setDepth(int x, int y, float z, const Vec3& color) {
		set(x, y, color);
		m_depth[index(x, y)] = z;
}
float Framebuffer::depthAt(int x, int y) const {
	return m_depth[index(x, y)];
}
