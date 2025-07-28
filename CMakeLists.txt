cmake_minimum_required(VERSION 3.31)
project(NotSpy__)

set(CMAKE_CXX_STANDARD 20)

add_executable(NotSpy__
        src/Shared.h
        src/HookDLL.cpp
        src/HookDLL.def
        src/MessageSpy.cpp
        helper/ListWindows.cpp)
