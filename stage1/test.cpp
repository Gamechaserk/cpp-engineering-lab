#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <iostream>

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    int* leak = new int(1);          // π “‚–π¬©£¨≤ª delete
    (void)leak;

    std::cout << "leaks detected: " << _CrtDumpMemoryLeaks() << "\n";
    return 0;
}