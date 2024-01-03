#include "gtest/gtest.h"
#include "../include/malloc.hpp"

TEST(MemoryTest, DefaultConstructor)
{
    Memory memory;
    void *allocatedMemory = memory.Alloc(1024);
    ASSERT_NE(allocatedMemory, nullptr);
    memory.Free(allocatedMemory);
}

TEST(MemoryTest, ConstructorWithSizeParameter)
{
    Memory memory(8 * KILOBYTE);
    void *allocatedMemory = memory.Alloc(2048);
    ASSERT_NE(allocatedMemory, nullptr);
    memory.Free(allocatedMemory);
}

TEST(MemoryTest, AllocationFailure)
{
    Memory memory(4 * KILOBYTE);
    void *allocatedMemory = memory.Alloc(8192);
    ASSERT_EQ(allocatedMemory, nullptr);
}

TEST(MemoryTest, FreeInvalidPointer)
{
    Memory memory;
    void *invalidPointer = reinterpret_cast<void *>(0xDEADBEEF);
    memory.Free(invalidPointer);
}

TEST(MemoryTest, Defragmentation)
{
    Memory memory(16 * KILOBYTE);
    void *allocatedMemory1 = memory.Alloc(4096);
    void *allocatedMemory2 = memory.Alloc(2048);
    memory.Free(allocatedMemory1);
    memory.Free(allocatedMemory2);
    // after freeing, the memory chunks should be defragmented into a single chunk.
    ASSERT_NE(memory.Alloc(6144), nullptr);
}

TEST(MemoryTest, InvalidMemoryFreeAfterDestructor)
{
    Memory *memory = new Memory;
    void *allocatedMemory = memory->Alloc(1024);
    delete memory;
    // Attempting to free memory after the destructor should not cause issues.
    memory->Free(allocatedMemory);
}
