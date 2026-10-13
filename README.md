# cpp-engineering-lab

C++ 工程化实践仓库：从工具链、CMake、内存模型开始，逐步走向游戏引擎方向。

当前内容（阶段一 · 工程化基础）：**手写智能指针 `SharedPtr` / `WeakPtr`**（模板 + 双计数控制块），配套 **GoogleTest + CTest** 测试。

## 目录结构

| 路径 | 内容 |
|---|---|
| `CMakeLists.txt` | 根构建脚本：编译选项、头文件库、测试目标、GoogleTest 拉取 |
| `include/my_shared_ptr.h` | `SharedPtr` / `WeakPtr` 实现（头文件库：模板代码必须随声明走） |
| `tests/test_shared_ptr.cpp` | 23 个 GoogleTest 用例（63 条断言）+ 自定义 `main`（配置 CRT 泄漏检查） |
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
23/23 Test #23: WeakPtr.BreaksCycle ..............   Passed    0.03 sec

100% tests passed out of 23
```

按名字筛选用例（`gtest_discover_tests` 带来的粒度）：

```powershell
ctest --test-dir build -C Debug -R WeakPtr              # 9 个 WeakPtr 相关用例
ctest --test-dir build -C Debug -R SharedPtr.MoveCtor   # 单个用例
```

也可以直接运行测试程序（用 gtest 自带的过滤器）：

```powershell
.\build\Debug\ptr_tests.exe
.\build\Debug\ptr_tests.exe --gtest_filter=WeakPtr.*
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

## 验证要点

- **23 个测试用例 / 63 条断言**，覆盖构造、拷贝、移动、赋值、自我赋值、`reset` 自保护、`WeakPtr` 全部路径
- **环形引用对照实验**——判据是**析构计数器**（不依赖任何外部工具）：

| 环上边的类型 | 节点析构数 | 结论 |
|---|---|---|
| `SharedPtr`（强引用成环） | **0** | 引用计数永不归零 → **内存泄漏** |
| `WeakPtr`（弱引用成环） | **2** | 节点正常析构 → **环被打破** |

> 说明：本项目用 `_CrtDumpMemoryLeaks()` 做辅助检查，但在**开启 ASan 的 Debug 版**下它恒返回 0（不产生有效证据），因此泄漏判据采用上表的析构计数器。