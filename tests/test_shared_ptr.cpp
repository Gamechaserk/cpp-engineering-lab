#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include "my_shared_ptr.h"
#include <gtest/gtest.h>

// ---------- 极简测试框架 ----------
static int g_cnode_destroyed = 0;

// ---------- 环形引用用节点（弱引用连边：应当不泄漏） ----------
class CNode {
public:
    WeakPtr<CNode> next;
    int value;
    CNode(int v) : value(v) {}
    ~CNode() { ++g_cnode_destroyed; }
};


// ============================================================
//  测试 1-14：SharedPtr 基础（v1 + v2）
// ============================================================
//测试1：构造
TEST(SharedPtr, Ctor) {
    SharedPtr<int> a(new int(42));
    EXPECT_EQ(a.use_count(), 1) << "构造后 use_count == 1";
    EXPECT_EQ(*a, 42) << "解引用得 42";
}
//测试2：拷贝构造
TEST(SharedPtr, CopyCtor) {
    SharedPtr<int> a(new int(42));
    SharedPtr<int> b = a;
    EXPECT_EQ(a.use_count(), 2) << "拷贝后 use_count == 2";
    EXPECT_EQ(a.get(), b.get()) << "共享同一个对象";
}
//测试3：其中一个先析构
TEST(SharedPtr, OneDestructsFirst) {
    SharedPtr<int> a(new int(7));
    {
        SharedPtr<int> b = a;
        EXPECT_EQ(a.use_count(), 2) << "内层 use_count == 2";
    }
    EXPECT_EQ(a.use_count(), 1) << "外层 use_count == 1";
    EXPECT_EQ(*a, 7) << "对象仍然有效（不崩）";
}
//测试4：reset
TEST(SharedPtr, Reset) {
    SharedPtr<int> a(new int(42));
    a.reset(new int(99));
    EXPECT_EQ(a.use_count(), 1) << "reset 后 use_count == 1";
    EXPECT_EQ(*a, 99) << "reset 后值为 99";
}
//测试5：reset(nullptr)
TEST(SharedPtr, ResetNull) {
    SharedPtr<int> a(new int(5));
    a.reset(nullptr);
    EXPECT_EQ(a.use_count(), 0) << "reset(nullptr) 后 use_count == 0";
    EXPECT_EQ(a.get(), nullptr) << "get() == nullptr";
}
//测试6：空对象 reset
TEST(SharedPtr, ResetOnEmpty) {
    SharedPtr<int> a;
    a.reset(new int(8));
    EXPECT_EQ(a.use_count(), 1) << "空对象 reset 后 use_count == 1";
    EXPECT_EQ(*a, 8) << "值为 8";
}
//测试7：拷贝赋值
TEST(SharedPtr, CopyAssign) {
    SharedPtr<int> a(new int(42));
    SharedPtr<int> b(new int(7));
    b = a;
    EXPECT_EQ(a.use_count(), 2) << "赋值后是 2";
    EXPECT_EQ(b.use_count(), 2) << "赋值后是 2";
}
//测试8：自我赋值
TEST(SharedPtr, SelfAssign) {
    SharedPtr<int> a(new int(42));
    SharedPtr<int>& ref = a;      // 用引用绕开编译器的自我赋值警告
    a = ref;
    EXPECT_EQ(a.use_count(), 1) << "自我赋值后 use_count == 1";
}
//测试9：链式赋值
TEST(SharedPtr, ChainedAssign) {
    SharedPtr<int> a(new int(42));
    SharedPtr<int> b(new int(7));
    SharedPtr<int> c(new int(8));
    a = b = c;
    EXPECT_EQ(a.use_count(), 3) << "链式赋值后是 3";
    EXPECT_EQ(b.use_count(), 3) << "链式赋值后是 3";
    EXPECT_EQ(c.use_count(), 3) << "链式赋值后是 3";
}
//测试10：赋空值
TEST(SharedPtr, AssignEmpty) {
    SharedPtr<int> a(new int(42));
    SharedPtr<int> empty;
    a = empty;
    EXPECT_EQ(a.use_count(), 0) << "赋空后 use_count == 0";
    EXPECT_EQ(a.get(), nullptr) << "get() == nullptr";
}
//测试11：赋值临时空对象
TEST(SharedPtr, AssignTempEmpty) {
    SharedPtr<int> a(new int(42));
    a = SharedPtr<int>();
    EXPECT_EQ(a.use_count(), 0) << "赋临时空对象后 use_count == 0";
}
//测试12：移动构造
TEST(SharedPtr, MoveCtor) {
    SharedPtr<int> a(new int(42));
    SharedPtr<int> b = std::move(a);
    EXPECT_FALSE(static_cast<bool>(a)) << "源对象被掏空";
    EXPECT_EQ(a.get(), nullptr) << "源对象 get() == nullptr";
    EXPECT_EQ(b.use_count(), 1) << "目标 use_count == 1（不是 2）";
    EXPECT_EQ(*b, 42) << "目标值 42";
}
//测试13：移动赋值
TEST(SharedPtr, MoveAssign) {
    SharedPtr<int> a(new int(42));
    SharedPtr<int> c(new int(7));
    c = std::move(a);
    EXPECT_EQ(a.use_count(), 0) << "源对象被掏空";
    EXPECT_EQ(c.use_count(), 1) << "目标 use_count == 1";
    EXPECT_EQ(*c, 42) << "目标值 42";
}
//测试14：自我 reset
TEST(SharedPtr, SelfReset) {
    SharedPtr<int> a(new int(42));
    a.reset(a.get());
    EXPECT_EQ(a.use_count(), 1) << "自我 reset 后 use_count == 1";
    EXPECT_EQ(*a, 42) << "值仍是 42（不是垃圾值）";
}

// ============================================================
//  测试 15-22：WeakPtr（v3）
// ============================================================
//测试15：WeakPtr 默认构造
TEST(WeakPtr, DefaultCtor) {
    WeakPtr<int> w;
    EXPECT_TRUE(w.expired()) << "空弱引用 expired() == true";
    EXPECT_EQ(w.lock(), nullptr) << "空弱引用 lock() 返回空";
}
//测试16：从 SharedPtr 构造
TEST(WeakPtr, FromSharedPtr) {
    SharedPtr<int> a(new int(42));
    WeakPtr<int> w(a);
    EXPECT_EQ(a.use_count(), 1) << "弱引用不增加强计数";
    EXPECT_FALSE(w.expired()) << "对象活着 → expired() == false";
}
//测试17：对象活着时 lock()
TEST(WeakPtr, LockAlive) {
    SharedPtr<int> a(new int(42));
    WeakPtr<int> w(a);
    {
        SharedPtr<int> sp = w.lock();
        ASSERT_TRUE(static_cast<bool>(sp)) << "lock() 成功";   // ← 失败就结束，避免下面解引用空指针
        EXPECT_EQ(*sp, 42) << "lock() 拿到正确对象";
        EXPECT_EQ(sp.use_count(), 2) << "lock 后 use_count == 2";
    }
    EXPECT_EQ(a.use_count(), 1) << "sp 析构后回到 1";
    EXPECT_FALSE(w.expired()) << "sp 析构后对象仍活着";
}
//测试18：对象死后 lock()
TEST(WeakPtr, LockAfterDeath) {
    SharedPtr<int> a(new int(42));
    WeakPtr<int> w(a);
    a.reset();
    EXPECT_TRUE(w.expired()) << "对象死后 expired() == true";
    SharedPtr<int> sp = w.lock();
    EXPECT_EQ(sp, nullptr) << "lock() 返回空";
}
//测试19：多个 WeakPtr 共存
TEST(WeakPtr, MultipleWeakPtrs) {
    WeakPtr<int> w1, w2;
    {
        SharedPtr<int> a(new int(7));
        WeakPtr<int> w3(a);
        w1 = w3;
        w2 = w3;
        EXPECT_EQ(a.use_count(), 1) << "多个弱引用，强计数仍为 1";
        EXPECT_TRUE(!w1.expired()&& !w3.expired()) << "w1/w3 都未过期";
    }
    EXPECT_TRUE(w1.expired()&& w2.expired()) << "a 死后 w1/w2 都过期";
    EXPECT_EQ(w1.lock(), nullptr) << "a 死后 lock() 返回空";
}
//测试20：WeakPtr 自我赋值
TEST(WeakPtr, SelfAssign) {
    SharedPtr<int> a(new int(1));
    WeakPtr<int> w(a);
    WeakPtr<int>& ref = w;
    w = ref;
    EXPECT_EQ(a.use_count(), 1) << "自我赋值后强计数仍为 1";
    EXPECT_FALSE(w.expired()) << "自我赋值后仍未过期";
}
//测试21：WeakPtr 移动语义
TEST(WeakPtr, MoveSemantics) {
    SharedPtr<int> a(new int(5));
    WeakPtr<int> w1(a);
    WeakPtr<int> w2 = std::move(w1);
    EXPECT_TRUE(w1.expired()) << "移动构造后源对象被掏空";
    EXPECT_FALSE(w2.expired()) << "目标有效";
    EXPECT_EQ(a.use_count(), 1) << "强计数不受影响";
    WeakPtr<int> w3;
    w3 = std::move(w2);
    EXPECT_FALSE(w3.expired()) << "移动赋值后目标有效";
}
//测试22：赋值一个空的 WeakPtr
TEST(WeakPtr, AssignEmpty) {
    SharedPtr<int> a(new int(9));
    WeakPtr<int> w(a);
    w = WeakPtr<int>();
    EXPECT_TRUE(w.expired()) << "赋空后 expired() == true";
    EXPECT_EQ(a.use_count(), 1) << "a 不受影响";
}

// ============================================================
//  测试 23：WeakPtr 打破环形引用
// ============================================================
TEST(WeakPtr, BreaksCycle) {
    g_cnode_destroyed = 0;
    {
        SharedPtr<CNode> a(new CNode(1));
        SharedPtr<CNode> b(new CNode(2));
        a->next = b;
        b->next = a;
        EXPECT_EQ(a.use_count(), 1) << "成环但强计数不增加";
        EXPECT_EQ(b.use_count(), 1) << "b 的强计数也是 1";
        EXPECT_FALSE(a->next.expired()) << "弱引用看到 b 还活着";

        SharedPtr<CNode> bx = a->next.lock();
        ASSERT_TRUE(static_cast<bool>(bx)) << "可以从弱引用 lock 出 b";
        EXPECT_EQ(b.use_count(), 2) << "lock 后 b 的强计数为 2";
        EXPECT_EQ(bx->value, 2) << "拿到的是 b";
        ASSERT_TRUE(static_cast<bool>(bx->next.lock())) << "还能沿环走回 a";
        EXPECT_EQ(bx->next.lock()->value, 1) << "环是通的";
    }
    EXPECT_EQ(g_cnode_destroyed, 2)
        << "两个节点都析构了 —— 环确实被打破";
}

int main(int argc, char** argv) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}