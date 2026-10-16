# cpp-engineering-lab

C++ 工程化实践仓库：从工具链、CMake、内存模型开始，逐步走向游戏引擎方向。

当前内容（阶段一 · 工程化基础）：

| 模块 | 内容 | 测试规模 |
|---|---|---|
| **手写智能指针** | `SharedPtr` / `WeakPtr`（模板 + 双计数控制块） | 23 用例 / 63 断言 |
| **动态数组** | `MyVector`（五法则 + 倍增扩容 + 迭代器） | 30 用例 |

配套 **GoogleTest + CTest**，每个模块一个测试可执行文件。

## 目录结构

| 路径 | 内容 |
|---|---|
| `CMakeLists.txt` | 根构建脚本：编译选项、头文件库、测试目标、GoogleTest 拉取 |
| `include/my_shared_ptr.h` | `SharedPtr` / `WeakPtr` 实现（头文件库：模板代码必须随声明走） |
| `include/my_vector.h` | `MyVector` 实现（同为头文件库） |
| `tests/test_shared_ptr.cpp` | 23 个 GoogleTest 用例（63 条断言）+ 自定义 `main`（配置 CRT 泄漏检查） |
| `tests/test_my_vector.cpp` | 30 个 GoogleTest 用例 + 自定义 `main` |
| `build/` | 构建产物，**不入版本库** |
| `README.md`、`.gitignore` | 说明与忽略规则 |

## 环境要求

| 组件 | 版本（开发环境实测） |
|---|---|
| Visual Studio 2022 | Community 17.11.5（需"使用 C++ 的桌面开发"工作负载） |
| MSVC 工具集 | 14.41.34120 |
| Windows SDK | 10.0.22621.0 |
| CMake | ≥ 3.20（开发用 4.4.3） |

## 构建

在**普通 PowerShell** 中执行即可，**不需要 VS 开发者环境**：

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

- 首次配置会**联网下载 GoogleTest**（约 10 秒），放在 `build/_deps/` 下；
- Debug 版开启 **AddressSanitizer**（`/fsanitize=address`）与 `/W4`；
- 构建后会把 ASan 运行时 DLL 自动复制到可执行文件旁边，因此**测试可以直接在普通 PowerShell 里运行**。

## 测试

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

期望结尾（实测输出）：

```text
53/53 Test #53: MyVector.BeginEndOnConstObject ................   Passed    0.06 sec

100% tests passed out of 53
```

按名字筛选用例（`gtest_discover_tests` 带来的粒度）：

```powershell
ctest --test-dir build -C Debug -R WeakPtr                 # 9 个 WeakPtr 相关用例
ctest --test-dir build -C Debug -R SharedPtr.MoveCtor      # 单个用例
ctest --test-dir build -C Debug -R MyVector                # 30 个 MyVector 用例
```

也可以直接运行测试程序（**每个模块一个 exe**，用 gtest 自带的过滤器）：

```powershell
.\build\Debug\ptr_tests.exe
.\build\Debug\ptr_tests.exe --gtest_filter=WeakPtr.*
.\build\Debug\vector_tests.exe --gtest_filter=MyVector.PushBack*
```

## 模块说明：手写智能指针

- `template <typename T>` 模板化，支持任意类型
- **五法则完整**：拷贝构造 / 拷贝赋值 / 移动构造 / 移动赋值 / 析构
- 自我赋值保护；`reset()` 自保护（`a.reset(a.get())` 安全）
- **双计数控制块** `ControlBlock<T>{ shared_count, weak_count, ptr }`
  - `shared_count == 0` → 销毁被管理对象
  - `weak_count == 0` → 销毁控制块本身
- **`WeakPtr`**：`lock()` / `expired()`，用于打破环形引用
- 指针语义：`operator->` / `operator*` / `explicit operator bool` / 与 `nullptr` 比较

### 验证要点：手写智能指针

- **23 个测试用例 / 63 条断言**，覆盖构造、拷贝、移动、赋值、自我赋值、`reset` 自保护、`WeakPtr` 全部路径
- **环形引用对照实验**——判据是**析构计数器**（不依赖任何外部工具）：

| 环上边的类型 | 节点析构数 | 结论 |
|---|---|---|
| `SharedPtr`（强引用成环） | **0** | 引用计数永不归零 → **内存泄漏** |
| `WeakPtr`（弱引用成环） | **2** | 节点正常析构 → **环被打破** |

> 说明：本项目用 `_CrtDumpMemoryLeaks()` 做辅助检查，但在**开启 ASan 的 Debug 版**下它恒返回 0（不产生有效证据），因此泄漏判据采用上表的析构计数器。
>
> 补充（U02 实测）：**ASan 本身确认在工作**——故意读取已释放内存时，它给出了完整的 `heap-use-after-free` 报告（含"分配点 / 释放点 / 使用点"三段调用链）。因此"CRT 泄漏检查恒 0"更可能是 **ASan 接管了 `operator new`、CRT 调试堆因而为空**；这一条**尚未直接确证**（待补对照实验），目前仍以析构计数器为准。

## 模块说明：MyVector（U02 · 动态数组 + 迭代器）

`include/my_vector.h` —— 手写的 `std::vector` 雏形。目的不是"再写一个容器"，而是把
**资源管理的五法则 + 扩容策略 + 迭代器契约** 一次性吃透，并用测试把契约钉住。

**实现要点**

| 主题 | 做法 | 为什么 |
|---|---|---|
| 扩容 | 容量**倍增**（`0 → 1 → 2 → 4 → 8 …`），单个 `reallocate()` 是全类**唯一**的扩容路径 | 倍增 ⇒ `push_back` 摊还 O(1)；唯一路径 ⇒ 不会"改一处漏一处" |
| 交付顺序 | 所有"分配 + 改状态"都是 **commit-last**：`new` → 搬元素 → **最后**才设 `size/capacity` | 中途抛异常时对象仍是**一致的旧状态**，不会出现"capacity 说有 10、其实没分配" |
| 拷贝 / 移动 | 拷贝 = 深拷贝（各自独立缓冲区）；移动 = **偷指针 + 把源置空**（`noexcept`） | 移动不产生新对象；源置空才能避免"偷完又删"的 use-after-free |
| 迭代器 | 直接暴露 `T*`（`T*` 本身就是随机访问迭代器）；`begin/end` 各 const 与非 const 两版 + `cbegin/cend` | `[begin, end)` 半开区间；const 版让 const 对象也能遍历 |
| 元素存储 | `new T[capacity]` —— **所有槽位都被构造** | ⚠️ 与 `std::vector`（裸存储 + placement new，只构造 `size` 个）**不同**：本项目不变式是 `alive == capacity()`，且要求 `T` 可默认构造 |

**验收**

| 命令 | 期望 |
|---|---|
| `ctest --test-dir build -C Debug --output-on-failure` | **53/53 通过**（U01 的 23 + U02 的 30） |
| `ctest --test-dir build -C Debug -R MyVector` | **30/30 通过** |

**被测试守住的不变式**

- 容量序列恒为 `1, 2, 4, 8, …, 128`（`PushBackGrowthIsObservable`）
- `Tracked::alive == capacity()`（`AliveEqualsCapacityAfterGrowth`）
- 深拷贝后 `a.data() != b.data()`（`CopyConstructorIsDeep`）
- 扩容后 `data()` 必变、旧元素完好（`DataPointerChangesAfterGrow`）
- 移动后源容器 `size()==0 && data()==nullptr`（`MoveConstructorStealsOwnership`）
- 作用域结束 `alive == 0`（`DestructionReleasesEverything`）
- 右值**恰好一次** move、左值**恰好一次** copy（`PushBackRvalueUsesMove`）

> ⚠️ **迭代器失效**：扩容会换缓冲区 → **扩容前拿到的指针 / 迭代器 / 引用全部失效**
> （再用 = `heap-use-after-free`，已用 ASan 实测确证）。这不是 bug，是契约。