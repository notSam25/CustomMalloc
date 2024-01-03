#include "malloc.hpp"

/**
 * @brief Construct a new Memory:: Memory object.
 * This default constructor has self-imposed limits of 4KB of memory.
 * See other constructor(s) for additional memory if needed.
 */
Memory::Memory()
{
    this->m_MapSize = 4 * KILOBYTE;

    void *result = mmap(
        nullptr,
        this->m_MapSize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);

    if (result == MAP_FAILED)
    {
        std::cerr << "[!] Failed to map memory\nerrno: " << errno << std::endl;
        return;
    }

    this->m_MapStart = result;
    std::cout << "[+] Successfully mapped memory" << std::endl
              << "    start: " << std::hex << this->m_MapStart << std::endl
              << "    size:  0x" << std::hex << this->m_MapSize << std::endl;

    this->m_FreeMemoryChunks.push_back(MemoryChunk{
        .start = this->m_MapStart,
        .size = this->m_MapSize});
}

/**
 * @brief Construct a new Memory:: Memory object
 *
 * @param kilobytes The size in KB that should be allocated.
 * Must be a multiple of one KB.
 */
Memory::Memory(size_t kilobytes)
{
    if (kilobytes % KILOBYTE != 0)
    {
        std::cerr << "[!] Failed to map memory\nmap size not multiple of a KILOBYTE" << std::endl;
        return;
    }

    this->m_MapSize = kilobytes;

    void *result = mmap(
        nullptr,
        this->m_MapSize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);

    if (result == MAP_FAILED)
    {
        std::cerr << "Failed to map memory\nerrno: " << errno << std::endl;
        exit(-1);
    }

    this->m_MapStart = result;
    std::cout << "[+] Successfully mapped memory" << std::endl
              << "    start: " << std::hex << this->m_MapStart << std::endl
              << "    size:  0x" << std::hex << this->m_MapSize << std::endl;

    this->m_FreeMemoryChunks.push_back(MemoryChunk{
        .start = this->m_MapStart,
        .size = this->m_MapSize});
}

/**
 * @brief Destroy the Memory:: Memory object
 */
Memory::~Memory()
{
    int result = munmap(this->m_MapStart, this->m_MapSize);
    if (result == -1)
    {
        std::cerr << "Failed to unmap memory\nerrno: " << errno << std::endl;
        return;
    }

    std::cout << "[+] Successfully unmapped memory" << std::endl;
}

/**
 * @brief Allocates memory of specified size
 *
 * @param size size in bytes to allocate
 * @returns pointer to the region of memory.
 * nullptr if the memory class constructor has an error.
 * nullptr if the size is greater than the max size of the map.
 * nullptr if there is no memory left to allocate.
 *
 */
void *Memory::Alloc(size_t size)
{
    if (this->m_MapStart == nullptr)
    {
        std::cerr << "[!] Cannot allocate memory. Virtual address region is nullptr" << std::endl;
        return nullptr;
    }

    if (size > this->m_MapSize)
    {
        return nullptr;
    }

    if (this->m_FreeMemoryChunks.size() == 0)
    {
        return nullptr;
    }
    this->m_ThreadMutex.lock();

    for (size_t idx = 0; idx < this->m_FreeMemoryChunks.size(); idx++)
    {
        if (this->m_FreeMemoryChunks[idx].size == size)
        {
            // swap the free memory chunk to the occupied memory chunks
            this->m_OccupiedMemoryChunks.push_back(this->m_FreeMemoryChunks[idx]);
            this->m_FreeMemoryChunks.erase(this->m_FreeMemoryChunks.begin() + idx);

            // gtfo!
            break;
        }
        else if (this->m_FreeMemoryChunks[idx].size > size)
        {

            // calc the difference between the requested memory and the chunk
            size_t diff = this->m_FreeMemoryChunks[idx].size - size;

            MemoryChunk shrunkChunk{
                .start = this->m_FreeMemoryChunks[idx].start,
                .size = size};

            MemoryChunk resizedChunk{
                .start = static_cast<std::byte *>(this->m_FreeMemoryChunks[idx].start) + size,
                .size = diff};

            // remove the old chunk
            this->m_FreeMemoryChunks.erase(this->m_FreeMemoryChunks.begin() + idx);

            // push the resized chunk
            this->m_FreeMemoryChunks.push_back(resizedChunk);

            // push the shrunken chunk
            this->m_OccupiedMemoryChunks.push_back(shrunkChunk);

            // gtfo!
            break;
        }
        else
        {
            // we haven't found any memory chunks
            if (idx == this->m_FreeMemoryChunks.size() - 1)
            {
                this->m_ThreadMutex.unlock();
                return nullptr;
            }
        }
    }
    this->m_ThreadMutex.unlock();

    // if all goes well, then the last chunk in the occupied is the value we need to return
    return this->m_OccupiedMemoryChunks[this->m_OccupiedMemoryChunks.size() - 1].start;
}

/**
 * @brief Marks a pointer and its associated memory chunk as free.
 * Does NOT zero the memory afterword.
 *
 * @param ptr the pointer to the chunk to free.
 */
void Memory::Free(void *ptr)
{
    if (this->m_MapStart == nullptr)
    {
        std::cerr << "[!] Cannot free memory. Virtual address region is nullptr" << std::endl;
        return;
    }

    if (ptr == nullptr ||
        ptr < this->m_MapStart ||
        ptr > static_cast<std::byte *>(this->m_MapStart) + this->m_MapSize)
    {
        return;
    }

    this->m_ThreadMutex.lock();
    for (size_t idx = 0; idx < this->m_OccupiedMemoryChunks.size(); idx++)
    {
        if (ptr == this->m_OccupiedMemoryChunks[idx].start)
        {
            this->m_FreeMemoryChunks.push_back(this->m_OccupiedMemoryChunks[idx]);
            this->m_OccupiedMemoryChunks.erase(this->m_OccupiedMemoryChunks.begin() + idx);
        }
    }
    this->DefragmentMemory();

    this->m_ThreadMutex.unlock();
}

/**
 * @brief Attempts to defragment memory by combining memory chunks
 */
void Memory::DefragmentMemory()
{
    size_t idx = 0;
    while (idx < this->m_FreeMemoryChunks.size())
    {
        MemoryChunk *curChunk = &this->m_FreeMemoryChunks[idx];

        // look at the previous chunk
        if (idx > 0)
        {
            MemoryChunk *prevChunk = &this->m_FreeMemoryChunks[idx - 1];
            if (static_cast<std::byte *>(prevChunk->start) + prevChunk->size == curChunk->start)
            {
                prevChunk->size += curChunk->size;
                this->m_FreeMemoryChunks.erase(this->m_FreeMemoryChunks.begin() + idx);
                idx--;
                continue;
            }
        }

        if (idx + 1 < this->m_FreeMemoryChunks.size())
        {
            MemoryChunk *nextChunk = &this->m_FreeMemoryChunks[idx + 1];
            if (static_cast<std::byte *>(curChunk->start) + curChunk->size == nextChunk->start)
            {
                curChunk->size += nextChunk->size;
                this->m_FreeMemoryChunks.erase(this->m_FreeMemoryChunks.begin() + idx + 1);
                continue;
            }
        }

        idx++;
    }
}