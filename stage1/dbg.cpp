#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <iostream>

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    //  关键两行：把报告从"调试器通道"改到 stdout
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

    int* leak = new int(1);
    (void)leak;

    std::cout << "--- 调用 _CrtDumpMemoryLeaks() ---\n";
    std::cout << "返回 " << _CrtDumpMemoryLeaks() << "\n";
    return 0;
}