#define _CRTDBG_MAP_ALLOC
#include<iostream>
#include<cstdlib>
#include<crtdbg.h> 
#include <cstddef>      // std::nullptr_t
#include"my_shared_ptr.h"



int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	/*
	std::cout << "=== 测试1：构造 ===\n";
	{
		SharedPtr<int> a(new int(42));
		std::cout << "a.use_count() = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 42)\n";
	}   // a 出作用域，析构

	std::cout << "\n=== 测试2：拷贝 ===\n";
	{
		SharedPtr<int> a(new int(42));
		SharedPtr<int> b = a;                       // 拷贝构造
		std::cout << "a.use_count() = " << a.use_count() << "  (期望 2)\n";
		std::cout << "b.use_count() = " << b.use_count() << "  (期望 2)\n";
		std::cout << "a.get() == b.get() ? " << (a.get() == b.get()) << "  (期望 1)\n";
	}   // 两个都出作用域

	std::cout << "\n=== 测试3：其中一个先析构，另一个还能用 ===\n";
	{
		SharedPtr<int> a(new int(7));
		{
			SharedPtr<int> b = a;
			std::cout << "内层：use_count = " << a.use_count() << "  (期望 2)\n";
		}   // b 先析构，计数回到 1
		std::cout << "外层：use_count = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 7，且不崩)\n";
		// 如果这里能正常打印 7，说明 b 析构时没有误删对象 ← 这就是 v1 的核心验收
	}
	std::cout << "\n=== 测试4：reset ===\n";
	{
		SharedPtr<int> a(new int(42));
		std::cout << "reset 前：use_count = " << a.use_count() << "  (期望 1)\n";
		a.reset(new int(99));
		std::cout << "reset 后：use_count = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 99)\n";
	}

	std::cout << "\n=== 测试5：reset(nullptr) ===\n";
	{
		SharedPtr<int> a(new int(5));
		a.reset(nullptr);
		std::cout << "use_count = " << a.use_count() << "  (期望 0)\n";
		std::cout << "get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
	}

	std::cout << "\n=== 测试6：空对象 reset ===\n";
	{
		SharedPtr<int> a;
		a.reset(new int(8));
		std::cout << "use_count = " << a.use_count() << "  (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << "  (期望 8)\n";
	}
	std::cout << "\n=== 测试7：拷贝赋值基本功能 ===\n";
	{
		SharedPtr<int> a(new int(42));
		SharedPtr<int> b(new int(7));
		b = a;
		std::cout << "use_count = " << a.use_count() << "  (期望 2)\n";
		std::cout << "use_count = " << b.use_count() << "  (期望 2)\n";
	}
	std::cout << "\n=== 测试8：自我赋值 a = a ===\n";
	{
		SharedPtr<int> a(new int(42));
		a = a;
		std::cout << "use_count = " << a.use_count() << "  (期望 1)\n";
	}
	std::cout << "\n=== 测试9：链式赋值 a = b = c ===\n";
	{
		SharedPtr<int> a(new int(42));
		SharedPtr<int> b(new int(7));
		SharedPtr<int> c(new int(8));
		a = b = c;
		std::cout << "use_count = " << a.use_count() << "  (期望 3)\n";
		std::cout << "use_count = " << a.use_count() << "  (期望 3)\n";
		std::cout << "use_count = " << a.use_count() << "  (期望 3)\n";
	}
	std::cout << "\n=== 测试10：空值赋值 a = empty ===\n";
	{
		SharedPtr<int> a(new int(42));
		SharedPtr<int> empty;
		a = empty;
		std::cout << "use_count = " << a.use_count() << "  (期望 0)\n";
		std::cout << "get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
	}
	std::cout << "\n=== 测试11：赋值一个临时构造的空对象（更真实的场景）===\n";
	{
		SharedPtr<int> a(new int(42));
		a = SharedPtr<int>();        // 右侧是"刚造出来的空对象"，赋值后立刻死亡
		std::cout << "use_count = " << a.use_count() << "  (期望 0)\n";
	}
	std::cout << "\n=== 测试12：移动构造 ===\n";
	{
		SharedPtr<int> a(new int(42));
		std::cout << "移动前：a.use_count() = " << a.use_count() << "  (期望 1)\n";
		SharedPtr<int> b = std::move(a);                  // 移动构造
		std::cout << "移动后：a.use_count() = " << a.use_count() << "  (期望 0，被掏空)\n";
		std::cout << "移动后：a.get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
		std::cout << "移动后：b.use_count() = " << b.use_count() << "  (期望 1)\n";
		std::cout << "移动后：*b.get() = " << *b.get() << "  (期望 42)\n";
	}
	std::cout << "\n=== 测试13：移动赋值 ===\n";
	{
		SharedPtr<int> a(new int(42));
		SharedPtr<int> c(new int(7));
		c = std::move(a);                            // 移动赋值
		std::cout << "a.use_count() = " << a.use_count() << "  (期望 0)\n";
		std::cout << "a.get() == nullptr ? " << (a.get() == nullptr) << "  (期望 1)\n";
		std::cout << "c.use_count() = " << c.use_count() << "  (期望 1)\n";
		std::cout << "*c.get() = " << *c.get() << "  (期望 42)\n";
		// 关键：c 原来管的 7 必须被释放（由最后的 leaks detected 验证）
	}
	std::cout << "\n=== 测试14：自我 reset（危险操作）===\n";
	{
		SharedPtr<int> a(new int(42));
		std::cout << "reset 前：use_count = " << a.use_count() << "\n";
		a.reset(a.get());        // 危险！
		std::cout << "reset 后：use_count = " << a.use_count() << " (期望 1)\n";
		std::cout << "*a.get() = " << *a.get() << " (期望 42)\n";
	}
	*/

	// ==================== v3: WeakPtr 测试 ====================
	/*
std::cout << "\n=== 测试15：WeakPtr 默认构造（空）===\n";
{
	WeakPtr<int> w;
	std::cout << "w.expired() = " << w.expired() << "  (期望 1，空弱引用视为已过期)\n";
	std::cout << "lock() 返回空 ? " << (w.lock() == nullptr) << "  (期望 1，不崩)\n";
}

std::cout << "\n=== 测试16：从 SharedPtr 构造 WeakPtr ===\n";
{
	SharedPtr<int> a(new int(42));
	WeakPtr<int> w(a);
	std::cout << "a.use_count() = " << a.use_count() << "  (期望 1)  ★弱引用不增加强计数\n";
	std::cout << "w.expired() = " << w.expired() << "  (期望 0)\n";
}
	
std::cout << "\n=== 测试17：对象活着时 lock() 成功 ===\n";
{
	SharedPtr<int> a(new int(42));
	WeakPtr<int> w(a);
	{
		SharedPtr<int> sp = w.lock();
		std::cout << "lock 成功 ? " << (sp != nullptr) << "  (期望 1)\n";
		std::cout << "*sp = " << *sp << "  (期望 42)\n";
		std::cout << "sp.use_count() = " << sp.use_count() << "  (期望 2)\n";
		std::cout << "a.use_count()  = " << a.use_count() << "  (期望 2)\n";
	}   // sp 析构
	std::cout << "sp 析构后 a.use_count() = " << a.use_count() << "  (期望 1)\n";
	std::cout << "sp 析构后 w.expired() = " << w.expired() << "  (期望 0)\n";
}
std::cout << "\n=== 测试18：对象死后 lock() 返回空 ===\n";
{
	SharedPtr<int> a(new int(42));
	WeakPtr<int> w(a);
	a.reset();                                   // 对象死亡
	std::cout << "w.expired() = " << w.expired() << "  (期望 1)\n";
	SharedPtr<int> sp = w.lock();
	std::cout << "lock 返回空 ? " << (sp == nullptr) << "  (期望 1)\n";
}

std::cout << "\n=== 测试19：多个 WeakPtr 共存 ===\n";
{
	WeakPtr<int> w1;
	WeakPtr<int> w2;
	{
		SharedPtr<int> a(new int(7));
		WeakPtr<int> w3(a);
		w1 = w3;                                 // 拷贝赋值
		w2 = w3;                                 // 拷贝赋值
		std::cout << "a.use_count() = " << a.use_count() << "  (期望 1)  ★仍有 1\n";
		std::cout << "w1.expired() = " << w1.expired() << "  (期望 0)\n";
		std::cout << "w3.expired() = " << w3.expired() << "  (期望 0)\n";
	}   // a、w3 析构
	std::cout << "a 析构后 w1.expired() = " << w1.expired() << "  (期望 1)\n";
	std::cout << "a 析构后 w2.expired() = " << w2.expired() << "  (期望 1)\n";
	std::cout << "a 析构后 w1.lock() 空 ? " << (w1.lock() == nullptr) << "  (期望 1)\n";
}   // w1、w2 析构 → 控制块此时才该被删

std::cout << "\n=== 测试20：WeakPtr 自我赋值 ===\n";
{
	SharedPtr<int> a(new int(1));
	WeakPtr<int> w(a);
	w = w;                                       // 自我赋值
	std::cout << "a.use_count() = " << a.use_count() << "  (期望 1)\n";
	std::cout << "w.expired() = " << w.expired() << "  (期望 0)\n";
}

std::cout << "\n=== 测试21：WeakPtr 移动语义 ===\n";
{
	SharedPtr<int> a(new int(5));
	WeakPtr<int> w1(a);
	WeakPtr<int> w2 = std::move(w1);             // 移动构造
	std::cout << "w1 被掏空 ? " << (w1.expired()) << "  (期望 1，源对象空了)\n";
	std::cout << "w2.expired() = " << w2.expired() << "  (期望 0)\n";
	std::cout << "a.use_count() = " << a.use_count() << "  (期望 1)\n";

	WeakPtr<int> w3;
	w3 = std::move(w2);                          // 移动赋值
	std::cout << "w3.expired() = " << w3.expired() << "  (期望 0)\n";
}

std::cout << "\n=== 测试22：赋值一个空的 WeakPtr ===\n";
{
	SharedPtr<int> a(new int(9));
	WeakPtr<int> w(a);
	w = WeakPtr<int>();                          // 赋空
	std::cout << "w.expired() = " << w.expired() << "  (期望 1)\n";
	std::cout << "a.use_count() = " << a.use_count() << "  (期望 1，a 不受影响)\n";
}
*/

// ==================== 最终验收：WeakPtr 打破环形引用 ====================

class CNode {
public:
	WeakPtr<CNode> next;      // ★ 环的一条边用【弱引用】
	int value;
	CNode(int v) : value(v) {}
};

std::cout << "\n=== 测试23：WeakPtr 打破环形引用 ===\n";
{
	SharedPtr<CNode> a(new CNode(1));
	SharedPtr<CNode> b(new CNode(2));

	a->next = b;                                  // a→b（弱）
	b->next = a;                                  // b→a（弱）

	std::cout << "a.use_count() = " << a.use_count() << "  (期望 1)  ★强计数没有因成环而增加\n";
	std::cout << "b.use_count() = " << b.use_count() << "  (期望 1)\n";
	std::cout << "a->next.expired() = " << a->next.expired() << "  (期望 0，b 还活着)\n";

	SharedPtr<CNode> bx = a->next.lock();         // 通过弱引用拿到 b
	std::cout << "lock 拿到 b ? " << (bool)bx << "  (期望 1)\n";
	std::cout << "b.use_count() = " << b.use_count() << "  (期望 2)\n";
	std::cout << "bx->value = " << bx->value << "  (期望 2)\n";
	std::cout << "bx 还能沿环走回 a 吗 ? value = " << bx->next.lock()->value
		<< "  (期望 1，环是通的)\n";
}   // a、b（以及临时对象）析构 → 环不存在强引用闭包 → 都该被释放

std::cout << "\nleaks detected: " << _CrtDumpMemoryLeaks() << "  (期望 0 —— 这就是 v3 的意义)\n";

std::cout << "\nleaks detected: " << _CrtDumpMemoryLeaks() << "  (期望 0)\n";
	std::cout << "\n";
	std::cout << "leaks detected: " << _CrtDumpMemoryLeaks() << "  (期望 0)\n";
	return 0;
}


class Node {
public:
	SharedPtr<Node> next;
	int value;
	Node(int v) : value(v) {}
};

/*
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

	std::cout << "=== 模板化验证：SharedPtr<int> 仍可用 ===\n";
	{
		SharedPtr<int> a(new int(42));
		SharedPtr<int> b = a;
		std::cout << "a.use_count() = " << a.use_count() << " (期望 2)\n";
		std::cout << "*a = " << *a << " (期望 42)\n";
		std::cout << "a.get() != nullptr ? " << (a.get() != nullptr) << " (期望 1)\n";
	}

	std::cout << "\n=== operator-> 链式访问验证 ===\n";
	{
		SharedPtr<Node> a(new Node(1));
		SharedPtr<Node> b(new Node(2));
		a->next = b;

		std::cout << "a->value = " << a->value << " (期望 1)\n";
		std::cout << "a->next->value = " << a->next->value << " (期望 2)\n";       // 两层 ->
		std::cout << "(*a).next->value = " << (*a).next->value << " (期望 2)\n";   // * 和 -> 等价
		std::cout << "(*a).value = " << (*a).value << " (期望 1)\n";
		std::cout << "a.use_count() = " << a.use_count() << " (期望 1)\n";
		std::cout << "b.use_count() = " << b.use_count() << " (期望 2，a->next 也指着它)\n";
	}

	std::cout << "\n=== v3 前置实验：环形引用 ===\n";
	{
		SharedPtr<Node> a(new Node(1));
		SharedPtr<Node> b(new Node(2));
		a->next = b;
		b->next = a;
		std::cout << "a.use_count() = " << a.use_count() << " (期望 2)\n";
		std::cout << "b.use_count() = " << b.use_count() << " (期望 2)\n";
		std::cout << "a->value = " << a->value << "\n";
		std::cout << "b->value = " << b->value << "\n";
	}

	std::cout << "\nleaks detected: " << _CrtDumpMemoryLeaks() << " (期望：非 0！)\n";
}
*/