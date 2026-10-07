#define _CRTDBG_MAP_ALLOC
#include <iostream>
#include <cstdlib>
#include <crtdbg.h>
#include "my_shared_ptr.h"

// ---------- 极简测试框架 ----------
static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg)                                              \
    do {                                                              \
        if (cond) { ++g_pass; std::cout << "  [PASS] " << msg << "\n"; } \
        else      { ++g_fail; std::cout << "  [FAIL] " << msg << "\n"; } \
    } while (0)

// ---------- 环形引用用节点（强引用连边：应当泄漏） ----------
class Node {
public:
    SharedPtr<Node> next;
    int value;
    Node(int v) : value(v) {}
};

// ---------- 环形引用用节点（弱引用连边：应当不泄漏） ----------
class CNode {
public:
    WeakPtr<CNode> next;
    int value;
    CNode(int v) : value(v) {}
};

// ============================================================
//  测试 1-14：SharedPtr 基础（v1 + v2）
// ============================================================
static void test_01_14() {
    std::cout << "\n=== 测试1：构造 ===\n";
    {
        SharedPtr<int> a(new int(42));
        CHECK(a.use_count() == 1, "构造后 use_count == 1");
        CHECK(*a == 42, "解引用得 42");
    }

    std::cout << "\n=== 测试2：拷贝构造 ===\n";
    {
        SharedPtr<int> a(new int(42));
        SharedPtr<int> b = a;
        CHECK(a.use_count() == 2, "拷贝后 use_count == 2");
        CHECK(a.get() == b.get(), "共享同一个对象");
    }

    std::cout << "\n=== 测试3：其中一个先析构 ===\n";
    {
        SharedPtr<int> a(new int(7));
        {
            SharedPtr<int> b = a;
            CHECK(a.use_count() == 2, "内层 use_count == 2");
        }
        CHECK(a.use_count() == 1, "外层 use_count == 1");
        CHECK(*a == 7, "对象仍然有效（不崩）");
    }

    std::cout << "\n=== 测试4：reset ===\n";
    {
        SharedPtr<int> a(new int(42));
        a.reset(new int(99));
        CHECK(a.use_count() == 1, "reset 后 use_count == 1");
        CHECK(*a == 99, "reset 后值为 99");
    }

    std::cout << "\n=== 测试5：reset(nullptr) ===\n";
    {
        SharedPtr<int> a(new int(5));
        a.reset(nullptr);
        CHECK(a.use_count() == 0, "reset(nullptr) 后 use_count == 0");
        CHECK(a.get() == nullptr, "get() == nullptr");
    }

    std::cout << "\n=== 测试6：空对象 reset ===\n";
    {
        SharedPtr<int> a;
        a.reset(new int(8));
        CHECK(a.use_count() == 1, "空对象 reset 后 use_count == 1");
        CHECK(*a == 8, "值为 8");
    }

    std::cout << "\n=== 测试7：拷贝赋值 ===\n";
    {
        SharedPtr<int> a(new int(42));
        SharedPtr<int> b(new int(7));
        b = a;
        CHECK(a.use_count() == 2 && b.use_count() == 2, "赋值后两个都是 2");
    }

    std::cout << "\n=== 测试8：自我赋值 ===\n";
    {
        SharedPtr<int> a(new int(42));
        SharedPtr<int>& ref = a;      // 用引用绕开编译器的自我赋值警告
        a = ref;
        CHECK(a.use_count() == 1, "自我赋值后 use_count == 1");
    }

    std::cout << "\n=== 测试9：链式赋值 ===\n";
    {
        SharedPtr<int> a(new int(42));
        SharedPtr<int> b(new int(7));
        SharedPtr<int> c(new int(8));
        a = b = c;
        CHECK(a.use_count() == 3 && b.use_count() == 3 && c.use_count() == 3,
            "链式赋值后三个都是 3");
    }

    std::cout << "\n=== 测试10：赋空值 ===\n";
    {
        SharedPtr<int> a(new int(42));
        SharedPtr<int> empty;
        a = empty;
        CHECK(a.use_count() == 0, "赋空后 use_count == 0");
        CHECK(a.get() == nullptr, "get() == nullptr");
    }

    std::cout << "\n=== 测试11：赋值临时空对象 ===\n";
    {
        SharedPtr<int> a(new int(42));
        a = SharedPtr<int>();
        CHECK(a.use_count() == 0, "赋临时空对象后 use_count == 0");
    }

    std::cout << "\n=== 测试12：移动构造 ===\n";
    {
        SharedPtr<int> a(new int(42));
        SharedPtr<int> b = std::move(a);
        CHECK(a.use_count() == 0, "源对象被掏空");
        CHECK(a.get() == nullptr, "源对象 get() == nullptr");
        CHECK(b.use_count() == 1, "目标 use_count == 1（不是 2）");
        CHECK(*b == 42, "目标值 42");
    }

    std::cout << "\n=== 测试13：移动赋值 ===\n";
    {
        SharedPtr<int> a(new int(42));
        SharedPtr<int> c(new int(7));
        c = std::move(a);
        CHECK(a.use_count() == 0, "源对象被掏空");
        CHECK(c.use_count() == 1, "目标 use_count == 1");
        CHECK(*c == 42, "目标值 42");
    }

    std::cout << "\n=== 测试14：自我 reset ===\n";
    {
        SharedPtr<int> a(new int(42));
        a.reset(a.get());
        CHECK(a.use_count() == 1, "自我 reset 后 use_count == 1");
        CHECK(*a == 42, "值仍是 42（不是垃圾值）");
    }
}

// ============================================================
//  测试 15-22：WeakPtr（v3）
// ============================================================
static void test_15_22() {
    std::cout << "\n=== 测试15：WeakPtr 默认构造 ===\n";
    {
        WeakPtr<int> w;
        CHECK(w.expired(), "空弱引用 expired() == true");
        CHECK(!w.lock(), "空弱引用 lock() 返回空");
    }

    std::cout << "\n=== 测试16：从 SharedPtr 构造 ===\n";
    {
        SharedPtr<int> a(new int(42));
        WeakPtr<int> w(a);
        CHECK(a.use_count() == 1, "★弱引用不增加强计数");
        CHECK(!w.expired(), "对象活着 → expired() == false");
    }

    std::cout << "\n=== 测试17：对象活着时 lock() ===\n";
    {
        SharedPtr<int> a(new int(42));
        WeakPtr<int> w(a);
        {
            SharedPtr<int> sp = w.lock();
            CHECK((bool)sp, "lock() 成功");
            CHECK(*sp == 42, "lock() 拿到正确对象");
            CHECK(sp.use_count() == 2, "lock 后 use_count == 2");
        }
        CHECK(a.use_count() == 1, "sp 析构后回到 1");
        CHECK(!w.expired(), "sp 析构后对象仍活着");
    }

    std::cout << "\n=== 测试18：对象死后 lock() ===\n";
    {
        SharedPtr<int> a(new int(42));
        WeakPtr<int> w(a);
        a.reset();
        CHECK(w.expired(), "对象死后 expired() == true");
        SharedPtr<int> sp = w.lock();
        CHECK(!sp, "lock() 返回空");
    }

    std::cout << "\n=== 测试19：多个 WeakPtr 共存 ===\n";
    {
        WeakPtr<int> w1, w2;
        {
            SharedPtr<int> a(new int(7));
            WeakPtr<int> w3(a);
            w1 = w3;
            w2 = w3;
            CHECK(a.use_count() == 1, "★多个弱引用，强计数仍为 1");
            CHECK(!w1.expired() && !w3.expired(), "w1/w3 都未过期");
        }
        CHECK(w1.expired() && w2.expired(), "a 死后 w1/w2 都过期");
        CHECK(!w1.lock(), "a 死后 lock() 返回空");
    }

    std::cout << "\n=== 测试20：WeakPtr 自我赋值 ===\n";
    {
        SharedPtr<int> a(new int(1));
        WeakPtr<int> w(a);
        WeakPtr<int>& ref = w;
        w = ref;
        CHECK(a.use_count() == 1, "自我赋值后强计数仍为 1");
        CHECK(!w.expired(), "自我赋值后仍未过期");
    }

    std::cout << "\n=== 测试21：WeakPtr 移动语义 ===\n";
    {
        SharedPtr<int> a(new int(5));
        WeakPtr<int> w1(a);
        WeakPtr<int> w2 = std::move(w1);
        CHECK(w1.expired(), "移动构造后源对象被掏空");
        CHECK(!w2.expired(), "目标有效");
        CHECK(a.use_count() == 1, "强计数不受影响");

        WeakPtr<int> w3;
        w3 = std::move(w2);
        CHECK(!w3.expired(), "移动赋值后目标有效");
    }

    std::cout << "\n=== 测试22：赋值一个空的 WeakPtr ===\n";
    {
        SharedPtr<int> a(new int(9));
        WeakPtr<int> w(a);
        w = WeakPtr<int>();
        CHECK(w.expired(), "赋空后 expired() == true");
        CHECK(a.use_count() == 1, "a 不受影响");
    }
}

// ============================================================
//  测试 23：WeakPtr 打破环形引用（今天的最终验收）
// ============================================================
static void test_23_circle() {
    std::cout << "\n=== 测试23：WeakPtr 打破环形引用 ===\n";
    {
        SharedPtr<CNode> a(new CNode(1));
        SharedPtr<CNode> b(new CNode(2));

        a->next = b;
        b->next = a;

        CHECK(a.use_count() == 1, "★成环但强计数不增加");
        CHECK(b.use_count() == 1, "b 的强计数也是 1");
        CHECK(!a->next.expired(), "弱引用看到 b 还活着");

        SharedPtr<CNode> bx = a->next.lock();
        CHECK((bool)bx, "可以从弱引用 lock 出 b");
        CHECK(b.use_count() == 2, "lock 后 b 的强计数为 2");
        CHECK(bx->value == 2, "拿到的是 b");
        CHECK(bx->next.lock()->value == 1, "还能沿环走回 a（环是通的）");
    }
}

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

    test_01_14();
    test_15_22();
    test_23_circle();

    std::cout << "\n===== " << g_pass << " / " << (g_pass + g_fail) << " passed";
    if (g_fail) std::cout << "  (" << g_fail << " FAILED)";
    std::cout << " =====\n";

    std::cout << "leaks detected: " << _CrtDumpMemoryLeaks()
        << "  (期望 0 —— 环形引用被 WeakPtr 破解)\n";
    return g_fail == 0 ? 0 : 1;
}