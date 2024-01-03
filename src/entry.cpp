#include <iostream>
#include <malloc.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    Memory *memoryInstance = new Memory;

    int *ptr = static_cast<int *>(memoryInstance->Alloc(sizeof(int)));
    *ptr = 0xDEADBEEF;

    std::cout << std::hex << *ptr << std::endl;

    memoryInstance->Free(static_cast<void *>(ptr));
    delete memoryInstance;

    return 0;
}