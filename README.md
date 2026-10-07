# cpp-engineering-lab

C++ 工程化实践仓库：从工具链、CMake、内存模型开始，逐步走向游戏引擎方向。

## 目录结构

| 目录 | 内容 | 状态 |
|---|---|---|
| `stage1_cmake/` | CMake 3 文件工程（`/W4` + Debug 下 ASan） | ✅ 完成 |
| `stage1/` | **手写智能指针**：`SharedPtr` / `WeakPtr`（模板 + 双计数控制块） | ✅ 58 条断言全绿、零泄漏 |

## stage1：手写智能指针

### 特性

- `template <typename T>` 模板化，支持任意类型
- **五法则完整**：拷贝构造 / 拷贝赋值 / 移动构造 / 移动赋值 / 析构
- 自我赋值保护、`reset()` 自保护（`a.reset(a.get())` 安全）
- **双计数控制块** `ControlBlock<T>{shared_count, weak_count, ptr}`
  - `shared_count == 0` → 销毁被管理对象
  - `weak_count == 0` → 销毁控制块本身
- **`WeakPtr`**：`lock()` / `expired()`，用于打破环形引用
- 指针语义：`operator->` / `operator*` / `explicit operator bool` / 与 `nullptr` 比较

### 构建与运行

```bash
cd stage1
cmake -S . -B build
cmake --build build --config Release
./build/Release/ptr_test.exe       # 期望：58 / 58 passed，leaks detected: 0
```

Debug 构建（开 ASan，需在 VS 开发者环境中运行）：

```bash
cmake --build build --config Debug
```

### 验证要点

- **58 条断言 / 23 个测试**，覆盖构造 / 拷贝 / 移动 / 赋值 / 自我赋值 / `reset` 自保护 / `WeakPtr` 全部路径
- **环形引用对照实验**：
  - 成环的边用 `SharedPtr` 连接 → `leaks detected: 1`（对象与控制块均泄漏）
  - 成环的边改用 `WeakPtr` 连接 → **`leaks detected: 0`**
- 内存泄漏用 **CRT 调试堆**检测（Windows MSVC 的 ASan 不支持 LeakSanitizer）

## 环境

| 组件 | 版本 |
|---|---|
| Visual Studio | 2022 Community 17.11.5 |
| MSVC 工具集 | 14.41.34120 |
| Windows SDK | 10.0.22621.0 |
| CMake | 4.4.3 |