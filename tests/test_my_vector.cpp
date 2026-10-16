#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include "my_vector.h"
#include <gtest/gtest.h>
#include <type_traits>   // is_same（编译期断言）
#include <utility>       // std::move
#include <string>        // NonTrivialElementType

struct Tracked {
    static int alive;
    static int destroyed;
    Tracked() { ++alive; }
    Tracked(const Tracked&) { ++alive; }
    ~Tracked() { --alive; ++destroyed; }
};

struct MoveTracked {
    static int copies;
    static int moves;
    MoveTracked() = default;
    MoveTracked(const MoveTracked&) { ++copies; }
    MoveTracked(MoveTracked&&) noexcept { ++moves; }
    MoveTracked& operator=(const MoveTracked&) { ++copies; return *this; }
    MoveTracked& operator=(MoveTracked&&) noexcept { ++moves; return *this; }
};
int MoveTracked::copies = 0;
int MoveTracked::moves = 0;

// 在头文件里（类外）或测试文件里
int Tracked::alive = 0;
int Tracked::destroyed = 0;

// 测试1：空容器
TEST(MyVector, Empty) {
    MyVector<int> v;
    EXPECT_EQ(v.size(), 0) << "空容器 size()==0";
    EXPECT_EQ(v.capacity(), 0) << "空容器 capacity()==0";
    EXPECT_EQ(v.data(),nullptr) << "空容器 指针为空";
    EXPECT_TRUE(v.empty()) << "空容器为空";
}
//测试2：单元素push_back()
TEST(MyVector, PushBackToEmpty) {
    MyVector<int> v;
    v.push_back(7);
    EXPECT_EQ(v.size(), 1) << "1 次 push_back(7) 后 size()==1";
    EXPECT_EQ(v.capacity(), 1) << "1 次 push_back(7) 后 capacity()==1";
    EXPECT_EQ(v[0], 7) << "1 次 push_back(7) 后 v[0]==7";
}
//测试3：push 100 个不同值
TEST(MyVector, PushBackCheckingValues) {
    MyVector<int> v;
    for (int i = 0; i < 100; i++) v.push_back(i);
    EXPECT_EQ(v.size(), 100) << "100 次 push_back() 后 size()==100";
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(v[i], i) << "100 次 push_back() 后 v[i]==i";
    }
}
//测试4：push 100 个不同值检测capacity
TEST(MyVector, PushBackGrowthIsObservable) {
    MyVector<int> v;
    int n = 1;
    for (int i = 0; i < 100; i++) {
        v.push_back(i);
        if (n <= i) n *= 2;
        EXPECT_EQ(v.capacity(), n) << "i 次 push_back() 后 capacity()==n";
    }
}
//测试5：扩容触发
TEST(MyVector, PushBackKeepsOldElements) {
    MyVector<int> v;
    for (int i = 0; i < 8; i++) v.push_back(i);
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(v[i], i) << "8 次 push_back() 后 v[i]==i";
    }
    v.push_back(8);
    EXPECT_EQ(v.capacity(), 16) << "触发扩容";
    for (int i = 0; i < 9; i++) {
        EXPECT_EQ(v[i], i) << "9 次 push_back() 后 v[i]==i";
    }
}
//测试6：空容器 reserve()
TEST(MyVector, ReserveGrows) {
    MyVector<int> v;
    v.reserve(10);
    EXPECT_EQ(v.capacity(), 10) << "1 次 reserve(10) 后 capacity()==10";
    EXPECT_NE(v.data(), nullptr) << "真的分配了";
}
//测试7：重复reserve() capacity 不变
TEST(MyVector, ReserveNoOpWhenEqual) {
    MyVector<int> v;
    v.reserve(20);
    int* before = v.data();
    EXPECT_EQ(v.capacity(), 20) << "1 次 reserve(20) 后 capacity()==20";
    v.reserve(20);
    EXPECT_EQ(v.capacity(), 20) << "1 次 reserve(20) 后 capacity()==20";
    EXPECT_EQ(before, v.data()) << "地址不变";
}
//测试8：装5个后 reserve(4) capacity 不变
TEST(MyVector, ReserveNeverShrinks) {
    //MyVector<int> v(5,1);
    MyVector<int> v;
    v.reserve(5);
    for (int i = 0; i < 5; i++) v.push_back(i);
    EXPECT_EQ(v.size(), 5) << "size()==5";
    EXPECT_EQ(v.capacity(), 5) << "capacity()==5";
    v.reserve(4);
    EXPECT_EQ(v.capacity(), 5) << "reserve(size-1)后 capacity()==5";
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(v[i], i) << "5 次 push_back() 后 v[i]==i";
    }
}
//测试9：空容器 reserve(10)逻辑不构造
TEST(MyVector, ReserveDoesNotConstructElements) {
    MyVector<int> v;
    v.reserve(10);
    EXPECT_EQ(v.size(), 0) << "1 次 reserve(10) 后 size()==0";
    EXPECT_TRUE(v.empty()) << "1 次 reserve(10) 后 容器为空";
}
//测试10：空容器 reserve(10)
TEST(MyVector, ReserveThenPushBack) {
    MyVector<int> v;
    v.reserve(10);
    for (int i = 0; i < 10; i++) v.push_back(i);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(v[i], i) << "10 次 push_back() 后 v[i]==i";
    }
    EXPECT_EQ(v.size(), 10) << "10 次 push_back() 后 size()==10";
    EXPECT_EQ(v.capacity(), 10) << "10 次 push 没触发扩容";
}
//测试11：有3个元素时 reserve(50) → 3 个值不变、 size() 不变
TEST(MyVector, ReserveKeepsExistingData) {
    MyVector<int> v;
    for (int i = 0; i < 3; i++) v.push_back(i);
    v.reserve(50);
    EXPECT_EQ(v.size(), 3) << "1 次 reserve(50) 后 size()==3";
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(v[i], i) << "3 次 push_back() 后 v[i]==i";
    }
}
//测试12：依赖 begin() 获取首地址元素
TEST(MyVector, SubscriptReadWrite) {
    MyVector<int> v;
    v.push_back(9);
    EXPECT_EQ(*(v.begin()), 9) << "v[0]=9; ";
    v[0] = 42; 
    EXPECT_EQ(v[0], 42);
}
// ---------- 13 编译期契约：const 对象上能用 operator[] ----------
TEST(MyVector, SubscriptOnConstObject) {            // 关键
    MyVector<int> v;
    v.push_back(9);
    const MyVector<int>& cv = v;          // 用 const 引用把"写权限"关掉
    EXPECT_EQ(cv[0], 9) << "const 对象上能读";
    EXPECT_EQ(cv.size(), 1);
    // 编译期断言：返回类型必须是 const T&
    static_assert(std::is_same<decltype(cv[0]), const int&>::value,
        "const 版 operator[] 应返回 const T&");
}
//测试14：空容器begin()==end()
TEST(MyVector, BeginEndOnEmptyAreEqual) {
    MyVector<int> v;
    EXPECT_EQ(v.begin(), v.end()) << "空容器begin()==end()";
}
//测试15：end() - begin() == size()
TEST(MyVector, BeginEndDistanceEqualsSize) {
    MyVector<int> v;
    for (int i = 0; i < 5; i++) v.push_back(i);
    EXPECT_EQ(v.end() - v.begin(), v.size()) << "end() - begin() == size()";
}
//测试16：范围 for 求和
TEST(MyVector, IterateWithRangeFor) {
    MyVector<int> v;
    for (int i = 0; i < 5; i++) v.push_back(i);
    int sum = 0;
    for (auto num : v) sum += num;
    EXPECT_EQ(sum, 10) << "范围 for 求和等于10";
}
// ---------- 17 扩容 = 换缓冲区 ----------
TEST(MyVector, DataPointerChangesAfterGrow) {
    MyVector<int> v;
    v.push_back(1);                       // cap 0 -> 1
    int* before = v.data();
    EXPECT_EQ(v.capacity(), 1);
    v.push_back(2);                       // cap 1 -> 2，触发 reallocate
    EXPECT_EQ(v.capacity(), 2);
    EXPECT_NE(v.data(), before) << "扩容后必须是新缓冲区";
    // 为什么 EXPECT_NE 可靠：reallocate 先 new 后 delete[]，旧块在分配时还活着
    EXPECT_EQ(v[0], 1) << "旧数据必须搬过来";
    EXPECT_EQ(v[1], 2);
}
// ---------- 19 拷贝构造是深拷贝 ----------
TEST(MyVector, CopyConstructorIsDeep) {              // 关键
    ASSERT_EQ(Tracked::alive, 0) << "前提：进入时计数干净";
    {
        MyVector<Tracked> a;
        a.push_back(Tracked{});           // a: cap 1 -> alive 1
        MyVector<Tracked> b(a);           // 深拷贝：再 new T[1]
        EXPECT_EQ(Tracked::alive, 2) << "深拷贝后 alive 应翻倍";
        EXPECT_EQ(b.size(), 1);
        EXPECT_EQ(b.capacity(), 1);
        EXPECT_NE(a.data(), b.data()) << " 不共享缓冲区（浅拷贝时这里会相等）";
    }
    EXPECT_EQ(Tracked::alive, 0) << "作用域结束必须全部析构";
}
// ---------- 20 拷贝赋值必须带上 size / capacity ----------
TEST(MyVector, CopyAssignmentCopiesCapacityAndSize) {
    MyVector<Tracked> a;
    a.push_back(Tracked{});
    {
        MyVector<Tracked> c;
        c = a;
        EXPECT_EQ(c.size(), 1) << " 拷贝赋值后 size 必须正确";
        EXPECT_EQ(c.capacity(), 1) << " 拷贝赋值后 capacity 必须正确";
        EXPECT_EQ(Tracked::alive, 2);
        EXPECT_NE(c.data(), a.data()) << "深拷贝";
    }
    EXPECT_EQ(Tracked::alive, 1) << "c 析构后只剩 a";
    EXPECT_EQ(a.size(), 1) << "a 不受影响";
}
// ---------- 21 自我赋值安全 ----------
TEST(MyVector, SelfAssignmentIsSafe) {
    MyVector<Tracked> v;
    v.push_back(Tracked{});               // alive 1
    MyVector<Tracked>& r = v;             // 用引用绕开编译器的自赋值警告
    r = v;
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(Tracked::alive, 1) << " 不能泄漏、不能多构造";
}
// ---------- 22 拷贝赋值要释放目标原有缓冲区 ----------
TEST(MyVector, CopyAssignmentReleasesOldBuffer) {
    MyVector<Tracked> a;
    a.push_back(Tracked{});
    MyVector<Tracked> c;
    c.push_back(Tracked{});               // a + c -> alive 2
    EXPECT_EQ(Tracked::alive, 2);
    c = a;                                // c 的旧缓冲区必须被释放
    EXPECT_EQ(Tracked::alive, 2) << " 释放旧的 1 个 + 新建 1 个 = 仍是 2";
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.capacity(), 1);
}
// ---------- 23 移动构造：偷，不复制 ----------
TEST(MyVector, MoveConstructorStealsOwnership) {
    MyVector<Tracked> a;
    a.push_back(Tracked{});               // alive 1
    MyVector<Tracked> b(std::move(a));
    EXPECT_EQ(Tracked::alive, 1) << " 移动不应复制元素（alive 不能翻倍）";
    EXPECT_EQ(b.size(), 1);
    EXPECT_EQ(b.capacity(), 1);
    EXPECT_EQ(a.size(), 0) << "源被置空";
    EXPECT_EQ(a.capacity(), 0);
    EXPECT_EQ(a.data(), nullptr);
    EXPECT_TRUE(b.data() != nullptr);
}
// ---------- 24 移动赋值：释放自己的旧缓冲区 + 接管 ----------
TEST(MyVector, MoveAssignmentReleasesOldBuffer) {
    MyVector<Tracked> a;
    a.push_back(Tracked{});
    MyVector<Tracked> c;
    c.push_back(Tracked{});               // alive 2
    EXPECT_EQ(Tracked::alive, 2);
    c = std::move(a);
    EXPECT_EQ(Tracked::alive, 1) << " c 的旧缓冲区被释放，只剩接管的 1 个";
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(a.size(), 0);
    EXPECT_EQ(a.data(), nullptr);
    EXPECT_TRUE(c.data() != nullptr);
}
// ---------- 25 析构释放干净 ----------
TEST(MyVector, DestructionReleasesEverything) {
    ASSERT_EQ(Tracked::alive, 0);
    {
        MyVector<Tracked> v;
        for (int i = 0; i < 10; ++i) v.push_back(Tracked{});
        EXPECT_EQ(Tracked::alive, (int)v.capacity())
            << "你的实现 new T[cap]：alive 应等于 capacity";
    }
    EXPECT_EQ(Tracked::alive, 0) << " 容器销毁后一个都不能剩";
    EXPECT_GT(Tracked::destroyed, 0) << "证明析构函数真的被调用过";
}
// ---------- 26 把"全量构造"这个设计决策钉进测试 ----------
TEST(MyVector, AliveEqualsCapacityAfterGrowth) {     // 关键
    MyVector<Tracked> v;
    EXPECT_EQ(Tracked::alive, 0);
    v.reserve(10);
    EXPECT_EQ(Tracked::alive, 10) << " new T[10] 会把 10 个槽位全部构造";
    EXPECT_EQ((std::size_t)Tracked::alive, v.capacity());
    v.push_back(Tracked{});
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ((std::size_t)Tracked::alive, v.capacity()) << "不变式：alive == capacity()";
    v.push_back(Tracked{});
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ((std::size_t)Tracked::alive, v.capacity());
}
// ---------- 27 非平凡类型（专抓 memcpy 类错误）----------
TEST(MyVector, NonTrivialElementType) {              // 关键
    MyVector<std::string> v;
    for (int i = 0; i < 8; ++i)
        v.push_back(std::string("s") + std::to_string(i));   // 会触发扩容
    EXPECT_EQ(v.size(), 8);
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(v[i], "s" + std::to_string(i)) << "扩容后 string 内容必须完好";
    MyVector<std::string> w(v);            // 深拷贝
    EXPECT_EQ(w.size(), 8);
    EXPECT_NE(w.data(), v.data());
    w[0] = "changed";
    EXPECT_EQ(v[0], "s0") << " 两个容器必须互相独立";
    v.push_back("extra");                  // 走 T&& 重载（临时量）
    EXPECT_EQ(w.size(), 8) << "w 不受影响";
    EXPECT_EQ(v[7], "s7") << "扩容后旧 string 未被破坏";
}
// ---------- 28 嵌套容器 ----------
TEST(MyVector, NestedVectorCopies) {
    MyVector<MyVector<int>> outer;
    MyVector<int> inner;
    inner.push_back(1);
    inner.push_back(2);
    outer.push_back(inner);                // 内层也必须是深拷贝
    EXPECT_EQ(outer.size(), 1);
    EXPECT_EQ(outer[0].size(), 2);
    EXPECT_EQ(outer[0][0], 1);
    outer[0][0] = 99;
    EXPECT_EQ(inner[0], 1) << " 内层独立";
    MyVector<MyVector<int>> copy(outer);
    EXPECT_EQ(copy[0].size(), 2);
    EXPECT_EQ(copy[0][0], 99);
}
// ---------- 29 填充构造（你刚修好 new T[len] 的那个）----------
TEST(MyVector, FillConstructor) {
    MyVector<int> v(3, 7);
    EXPECT_EQ(v.size(), 3) << " 构造了 len 个元素，size 必须是 3";
    EXPECT_EQ(v.capacity(), 3);
    for (int i = 0; i < 3; ++i) EXPECT_EQ(v[i], 7);
}
// ---------- 30 右值重载真的在移动（精确版）----------
TEST(MyVector, PushBackRvalueUsesMove) {             // 关键
    MyVector<MoveTracked> v;
    v.reserve(4);                      //  先备好空间：消除扩容带来的干扰拷贝
    MoveTracked::copies = 0;           //  清零放在 reserve 之后
    MoveTracked::moves = 0;

    v.push_back(MoveTracked{});        // 临时量（右值）→ T&& 重载
    EXPECT_EQ(MoveTracked::moves, 1) << " 右值必须产生恰好一次 move";
    EXPECT_EQ(MoveTracked::copies, 0) << " 右值路径不该有任何 copy";

    MoveTracked lv;
    v.push_back(lv);                   // 左值 → const T& 重载
    EXPECT_EQ(MoveTracked::copies, 1) << " 左值必须产生恰好一次 copy";
    EXPECT_EQ(MoveTracked::moves, 1) << "左值不该产生新的 move";
    EXPECT_EQ(v.size(), 2);
}
// ---------- 31 const 对象上的迭代器 / 范围 for（专抓"漏 const 版"）----------
TEST(MyVector, BeginEndOnConstObject) {              // 关键
    MyVector<int> v;
    v.push_back(1);
    v.push_back(2);
    const MyVector<int>& cv = v;
    EXPECT_EQ(cv.end() - cv.begin(), 2);
    static_assert(std::is_same<decltype(cv.begin()), const int*>::value,
        "const 版 begin 应返回 const T*");
    int sum = 0;
    for (const auto& x : cv) sum += x;     //  const 对象上的范围 for 必须能编译
    EXPECT_EQ(sum, 3);
}


int main(int argc, char** argv) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}