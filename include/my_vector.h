#pragma once
#include<cstddef>
#include <utility>

template <typename T>
class MyVector {
public:
	MyVector() = default; //空构造
	explicit MyVector(std::size_t len,const T& value) { //有值构造
		m_data = new T[len];
		for (std::size_t i = 0; i < len; i++) {
			m_data[i] = value;
		}
		m_size = len;
		m_capacity = len;
	}
	MyVector(const MyVector& other) { //拷贝构造
		if (other.m_capacity != 0) {
			m_data = new T[other.m_capacity];
			for (std::size_t i = 0; i < other.m_size; i++) {
				m_data[i] = other.m_data[i];
			}
			m_capacity = other.m_capacity;
			m_size = other.m_size;
		}
	}
	MyVector& operator=(const MyVector& other) { //拷贝赋值
		if (this == &other) return *this;
		release();
		if (other.m_capacity != 0) {
			m_data = new T[other.m_capacity];
			for (std::size_t i = 0; i < other.m_size; i++) {
				m_data[i] = other.m_data[i];
			}
			m_size = other.m_size;
			m_capacity = other.m_capacity;
		}
		return *this;
	}
	MyVector(MyVector&& other) noexcept { //移动构造
		if (other.m_capacity != 0) {
			m_data = other.m_data;
			m_capacity = other.m_capacity;
			m_size = other.m_size;
		}
		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_size = 0;
	}
	MyVector& operator=(MyVector&& other) noexcept { //移动赋值
		if (this == &other) return *this;
		release();
		if (other.m_capacity != 0) {
			m_data = other.m_data;
			m_capacity = other.m_capacity;
			m_size = other.m_size;
			other.m_data = nullptr;
			other.m_capacity = 0;
			other.m_size = 0;
		}
		return *this;
	}
	~MyVector() {
		release();
	}

	std::size_t size() const { return m_size; }
	std::size_t capacity() const { return m_capacity; }
	bool empty() const { return m_size == 0; }

	void reserve(std::size_t n) { if (m_capacity < n)  reallocate(n); }

	T* data() { return m_data; }
	const T* data() const { return m_data; }
	T& operator[](std::size_t i) { return m_data[i]; }
	const T& operator[](std::size_t i) const { return m_data[i]; }
	T* begin() { return m_data; }
	const T* begin() const { return m_data; }
	const T* cbegin() const { return m_data; }
	T* end() { return m_data + m_size; }
	const T* end() const { return m_data + m_size; }
	const T* cend() const { return m_data + m_size; }

	void push_back(const T& target) {
		if (m_capacity <= m_size) {
			reallocate(m_capacity == 0 ? 1 : m_capacity * 2);
		}
		m_data[m_size] = target;
		m_size++;
	}
	void push_back(T&& target) {
		if (m_capacity <= m_size) {
			reallocate(m_capacity == 0 ? 1 : m_capacity * 2);
		}
		m_data[m_size] = std::move(target);
		m_size++;
	}

private:
	void release() {
		delete[] m_data;
		m_data = nullptr;
		m_capacity = 0;
		m_size = 0;
	}
	void reallocate(std::size_t new_cap) {
		T* tem_data = m_data;
		m_data = new T[new_cap];
		for (std::size_t i = 0; i < m_size; i++) {
			m_data[i] = tem_data[i];
		}
		delete[] tem_data;
		tem_data = nullptr;
		m_capacity = new_cap;
	}

	T* m_data = nullptr;
	std::size_t m_size = 0;
	std::size_t m_capacity = 0;
};