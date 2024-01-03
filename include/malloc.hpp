#pragma once
#include <cstddef>
#include <sys/mman.h>
#include <iostream>
#include <vector>
#include <mutex>

#define KILOBYTE 1024

class Memory
{
public:
    Memory();
    Memory(size_t kilobytes);

    ~Memory();

    void *Alloc(size_t size);
    void Free(void *ptr);

private:
    struct MemoryChunk
    {
        void *start;
        size_t size;
    };

    void DefragmentMemory();

    void *m_MapStart = nullptr;
    size_t m_MapSize = 0;
    std::vector<MemoryChunk> m_FreeMemoryChunks, m_OccupiedMemoryChunks;
    std::mutex m_ThreadMutex;
};