cmake_minimum_required(VERSION 4.1)
project(code_gen_tests)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library( code_gen_lib
        src/Assembler.cpp
        include/Assembler.h
        src/FunctionHandle.cpp
        include/FunctionHandle.h
        src/CodeBuffer.cpp
        include/CodeBuffer.h
        src/utils.cpp
        include/constants/opcode_utils.h
        src/GeneratedCode.cpp
        include/GeneratedCode.h
        src/Relocation.cpp
        include/Relocation.h
        include/constants/Registers.h
        src/LinkingInfo.cpp
        include/LinkingInfo.h
        src/TBuffer.cpp
        include/TBuffer.h
        src/Async.cpp
        include/Async.h
        include/MemoryAddress.h
        include/constants/RegSize.h
        include/Segment.h
        src/Segment.cpp
        src/Stacks.cpp
        include/Stacks.h)

add_executable(code_gen_tests src/main.cpp)
target_link_libraries(code_gen_tests code_gen_lib)

add_subdirectory(libs/googletest)
add_executable(run_tests
        tests/tests.cpp)
target_include_directories(run_tests PUBLIC libs/googletest/include)
target_link_libraries(run_tests gtest gtest_main code_gen_lib)