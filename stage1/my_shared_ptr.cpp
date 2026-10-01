#define _CRTDBG_MAP_ALLOC
#include<iostream>
#include<cstdlib>
#include<crtdbg.h> 

class SharedPtr {
public:
	explicit SharedPtr(int* p = nullptr)
		:m_ptr(p), m_cb(nullptr)
	{
		if (m_ptr != nullptr) {
			m_cb=new long(1);
		}
	}

	SharedPtr(const SharedPtr& other)
		:m_ptr(other.m_ptr), m_cb(other.m_cb)
	{
		if(other.m_cb!=nullptr)
			(*m_cb)++;
	}
	SharedPtr& operator=(const SharedPtr& other) {
		if (this == &other) return *this;
		release();
		m_ptr = other.m_ptr;
		m_cb = other.m_cb;
		if (other.m_cb != nullptr) {			
			(*m_cb)++;
		}
		return *this;
	}
	SharedPtr(SharedPtr&& other) noexcept {
		m_ptr = other.m_ptr;
		m_cb = other.m_cb;
		other.m_cb = nullptr;
		other.m_ptr = nullptr;
	}

	SharedPtr& operator=(SharedPtr&& other) noexcept {
		if (this == &other) {
			return *this;
		}
		release();
		m_ptr = other.m_ptr;
		m_cb = other.m_cb;
		other.m_cb = nullptr;
		other.m_ptr = nullptr;
		return *this;
	}
	void reset(int* ptr=nullptr) {	//待处理a.reset(a.get());
		release();
		m_ptr = ptr;
		if (m_ptr != nullptr) {
			m_cb = new long(1);
		}
		else m_cb = nullptr;
	}

	~SharedPtr() {
		release();
	}

	int* get() const {
		return m_ptr;
	}
	long use_count() const {
		return m_cb ? *m_cb : 0;
	}
	
private:

	void release() {
		if (m_cb == nullptr) return;
		--(*m_cb);
		if (*m_cb == 0) {
			delete m_ptr;
			delete m_cb;
			m_ptr = nullptr;
			m_cb = nullptr;
		}
	}
	int* m_ptr = nullptr;
	long* m_cb = nullptr;
};




int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	std::cout << "=== 测试1：构造 ===\n";
	{
		SharedPtr a(new int(42));
		std::cout << "a.use_count() = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 42)\n";
	}   // a 出作用域，析构

	std::cout << "\n=== 测试2：拷贝 ===\n";
	{
		SharedPtr a(new int(42));
		SharedPtr b = a;                       // 拷贝构造
		std::cout << "a.use_count() = " << a.use_count() << "  (期望 2)\n";
		std::cout << "b.use_count() = " << b.use_count() << "  (期望 2)\n";
		std::cout << "a.get() == b.get() ? " << (a.get() == b.get()) << "  (期望 1)\n";
	}   // 两个都出作用域

	std::cout << "\n=== 测试3：其中一个先析构，另一个还能用 ===\n";
	{
		SharedPtr a(new int(7));
		{
			SharedPtr b = a;
			std::cout << "内层：use_count = " << a.use_count() << "  (期望 2)\n";
		}   // b 先析构，计数回到 1
		std::cout << "外层：use_count = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 7，且不崩)\n";
		// 如果这里能正常打印 7，说明 b 析构时没有误删对象 ← 这就是 v1 的核心验收
	}
	std::cout << "\n=== 测试4：reset ===\n";
	{
		SharedPtr a(new int(42));
		std::cout << "reset 前：use_count = " << a.use_count() << "  (期望 1)\n";
		a.reset(new int(99));
		std::cout << "reset 后：use_count = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 99)\n";
	}

	std::cout << "\n=== 测试5：reset(nullptr) ===\n";
	{
		SharedPtr a(new int(5));
		a.reset(nullptr);
		std::cout << "use_count = " << a.use_count() << "  (期望 0)\n";
		std::cout << "get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
	}

	std::cout << "\n=== 测试6：空对象 reset ===\n";
	{
		SharedPtr a;
		a.reset(new int(8));
		std::cout << "use_count = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 8)\n";
	}
	std::cout << "\n=== 测试7：拷贝赋值基本功能 ===\n";
	{
		SharedPtr a(new int(42));
		SharedPtr b(new int(7));
		b = a;
		std::cout << "use_count = " << a.use_count() << "  (期望 2)\n";
		std::cout << "use_count = " << b.use_count() << "  (期望 2)\n";
	}
	std::cout << "\n=== 测试8：自我赋值 a = a ===\n";
	{
		SharedPtr a(new int(42));
		a = a;
		std::cout << "use_count = " << a.use_count() << "  (期望 1)\n";
	}
	std::cout << "\n=== 测试9：链式赋值 a = b = c ===\n";
	{
		SharedPtr a(new int(42));
		SharedPtr b(new int(7));
		SharedPtr c(new int(8));
		a = b = c;
		std::cout << "use_count = " << a.use_count() << "  (期望 3)\n";
		std::cout << "use_count = " << a.use_count() << "  (期望 3)\n";
		std::cout << "use_count = " << a.use_count() << "  (期望 3)\n";
	}
	std::cout << "\n=== 测试10：空值赋值 a = empty ===\n";
	{
		SharedPtr a(new int(42));
		SharedPtr empty;
		a = empty;
		std::cout << "use_count = " << a.use_count() << "  (期望 0)\n";
		std::cout << "get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
	}
	std::cout << "\n=== 测试11：赋值一个临时构造的空对象（更真实的场景）===\n";
	{
		SharedPtr a(new int(42));
		a = SharedPtr();        // 右侧是"刚造出来的空对象"，赋值后立刻死亡
		std::cout << "use_count = " << a.use_count() << "  (期望 0)\n";
	}
	std::cout << "\n=== 测试12：移动构造 ===\n";
	{
		SharedPtr a(new int(42));
		std::cout << "移动前：a.use_count() = " << a.use_count() << "  (期望 1)\n";
		SharedPtr b = std::move(a);                  // 移动构造
		std::cout << "移动后：a.use_count() = " << a.use_count() << "  (期望 0，被掏空)\n";
		std::cout << "移动后：a.get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
		std::cout << "移动后：b.use_count() = " << b.use_count() << "  (期望 1)\n";
		std::cout << "移动后：*b.get() = " << *b.get() << "  (期望 42)\n";
	}
	std::cout << "\n=== 测试13：移动赋值 ===\n";
	{
		SharedPtr a(new int(42));
		SharedPtr c(new int(7));
		c = std::move(a);                            // 移动赋值
		std::cout << "a.use_count() = " << a.use_count() << "  (期望 0)\n";
		std::cout << "a.get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
		std::cout << "c.use_count() = " << c.use_count() << "  (期望 1)\n";
		std::cout << "*c.get() = " << *c.get() << "  (期望 42)\n";
		// 关键：c 原来管的 7 必须被释放（由最后的 leaks detected 验证）
	}
	std::cout << "\n";
	std::cout << "leaks detected: " << _CrtDumpMemoryLeaks() << "  (期望 0)\n";
	return 0;
}