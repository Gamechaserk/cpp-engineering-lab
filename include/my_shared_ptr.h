#pragma once
#include<cstddef>

template <typename T>
struct ControlBlock {
	long shared_count;      // 强计数：有多少个 SharedPtr
	long weak_count;        // 弱计数：有多少个"需要控制块活着"的引用
	T* ptr;                 // 对象指针（新加的！）
};


template<typename T> class WeakPtr;


template<typename T>
class SharedPtr {
public:
	explicit SharedPtr(T* p = nullptr) //构造函数
	{
		if (p != nullptr) {
			m_cb =new ControlBlock<T>();
			m_cb->shared_count = 1;
			m_cb->weak_count = 1;
			m_cb->ptr = p;
		}
	}

	SharedPtr(const SharedPtr& other) //拷贝构造函数
		:m_cb(other.m_cb)
	{
		if (other.m_cb != nullptr) {
			(m_cb->shared_count)++;
			(m_cb->weak_count)++;
		}
	}

	SharedPtr& operator=(const SharedPtr& other) { //拷贝赋值
		if (this == &other) return *this;
		release();
		m_cb = other.m_cb;
		if (other.m_cb != nullptr) {
			(m_cb->shared_count)++;
			(m_cb->weak_count)++;
		}
		return *this;
	}

	SharedPtr(SharedPtr&& other) noexcept { //移动构造
		m_cb= other.m_cb;
		other.m_cb = nullptr;
	}

	SharedPtr& operator=(SharedPtr&& other) noexcept { //移动赋值
		if (this == &other) {
			return *this;
		}
		release();
		m_cb = other.m_cb;
		other.m_cb = nullptr;
		return *this;
	}

	// 与 nullptr 比较（提供 sp == nullptr / != 的反向写法）
	friend bool operator==(const SharedPtr& sp, std::nullptr_t) noexcept { return sp.m_cb == nullptr; }
	friend bool operator==(std::nullptr_t, const SharedPtr& sp) noexcept { return sp.m_cb == nullptr; }
	friend bool operator!=(const SharedPtr& sp, std::nullptr_t) noexcept { return sp.m_cb != nullptr; }
	friend bool operator!=(std::nullptr_t, const SharedPtr& sp) noexcept { return sp.m_cb != nullptr; }

	T* operator->() const { return m_cb->ptr; }

	T& operator*() const { return *(m_cb->ptr); }

	explicit operator bool() const { return m_cb != nullptr && m_cb->ptr != nullptr; }

	void reset(T* p = nullptr) {	
		if (m_cb && m_cb->ptr == p) return;
		release();
		if (p != nullptr) {
			m_cb = new ControlBlock<T>();
			m_cb->ptr = p;
			m_cb->shared_count = 1;
			m_cb->weak_count = 1;
		}
		else m_cb = nullptr;
	}

	~SharedPtr() { release(); }

	T* get() const { return m_cb ? m_cb->ptr : nullptr; }

	long use_count() const { return m_cb ? m_cb->shared_count : 0; }

private:

	void release() {
		if (m_cb == nullptr) return;
		--(m_cb->shared_count);
		if (m_cb->shared_count == 0) {
			delete m_cb->ptr;
			m_cb->ptr = nullptr;
		}
		--(m_cb->weak_count);
		if (m_cb->weak_count == 0) {
			delete m_cb;
			m_cb = nullptr;
		}
	}
	explicit SharedPtr(ControlBlock<T>* cb) : m_cb(cb) {}
	ControlBlock<T>* m_cb = nullptr;
	friend class WeakPtr<T>;
};


template<typename T>
class WeakPtr {
public:
	WeakPtr() = default; //空弱引用

	WeakPtr(const SharedPtr<T>& sp) { //从shared构造
		if (sp.m_cb != nullptr) {
			m_cb = sp.m_cb;
			m_cb->weak_count++;
		}
	}

	WeakPtr(const WeakPtr& other) //拷贝构造
		:m_cb(other.m_cb)
	{ 
		if (other.m_cb != nullptr) {
			m_cb->weak_count++;
		}
	}

	WeakPtr& operator=(const WeakPtr& other) { //拷贝赋值
		if (this == &other) return *this;
		release();//依赖release无条件置空
		if (other.m_cb != nullptr) {
			m_cb = other.m_cb;
			m_cb->weak_count++;
		}
		return *this;
	}

	~WeakPtr() { release(); }

	SharedPtr<T> lock() const { 
		if (m_cb == nullptr) return SharedPtr<T>();
		if (m_cb->ptr == nullptr) return SharedPtr<T>();
		m_cb->shared_count++;
		m_cb->weak_count++;
		return SharedPtr<T>(m_cb);
	}

	WeakPtr(WeakPtr&& other) noexcept { //移动构造
		m_cb = other.m_cb;
		other.m_cb = nullptr;      
	}

	WeakPtr& operator=(WeakPtr&& other) noexcept { //移动赋值
		if (this == &other) return *this;
		release();
		m_cb = other.m_cb;
		other.m_cb = nullptr;
		return *this;
	}

	bool expired() const { return m_cb == nullptr || m_cb->shared_count == 0; }

private:
	void release() {
		if (m_cb == nullptr) return;
		--(m_cb->weak_count);
		if (m_cb->weak_count == 0) {
			delete m_cb;
		}
		m_cb = nullptr;
	}

	ControlBlock<T>* m_cb = nullptr;
};