# test/CMakeLists.txt

# Add the executable for your tests
add_executable(test_memory test_memory.cpp)

# Link the test executable with the project library and testing libraries
target_link_libraries(test_memory PRIVATE custom_malloc_impl_library gtest_main)

# Add the test to CTest
add_test(NAME MemoryTest COMMAND test_memory)
