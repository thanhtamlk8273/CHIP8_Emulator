cmake_minimum_required(VERSION 3.5)

project(CHIP8Emu LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ASAN
set(ENABLE_ASAN OFF CACHE BOOL "Enable to compile with ASAN")
set(ASAN_COMPILE_OPTIONS -fsanitize=address -fno-omit-frame-pointer)
set(ASAN_LINKER_OPTIONS -fsanitize=address)

if(ENABLE_ASAN)
    message(STATUS "Binary will be compiled with ASAN")
else()
    message(STATUS "Binary will be compiled without ASAN")
    set(ASAN_COMPILE_OPTIONS )
    set(ASAN_LINKER_OPTIONS )
endif(ENABLE_ASAN)
unset(ENABLE_ASAN CACHE)

set(CMAKE_PREFIX_PATH "/home/xemanon/Qt/6.4.2/gcc_64")
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

enable_testing()
include(GoogleTest)

add_subdirectory(googletest)
add_subdirectory(tests)

add_subdirectory(fmt)

include_directories(
    src
)

set(SRCS
    src/main.cpp
    src/chip8_pc.cpp
    src/chip8_pc.h
    src/monitor.cpp
    src/monitor.h
    src/numberdisplay.cpp
    src/numberdisplay.h
)

add_executable(CHIP8Emu ${SRCS})
target_link_libraries(CHIP8Emu PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
target_link_libraries(CHIP8Emu PRIVATE gtest gmock gtest_main)
target_link_libraries(CHIP8Emu PRIVATE fmt::fmt)
target_compile_options(CHIP8Emu PRIVATE -Wall -g -O2)
target_compile_options(CHIP8Emu PRIVATE ${ASAN_COMPILE_OPTIONS})
target_link_options(CHIP8Emu PRIVATE ${ASAN_LINKER_OPTIONS})

install(TARGETS CHIP8Emu
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
