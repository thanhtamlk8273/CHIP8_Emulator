# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.
#add_subdirectory("${PROJECT_SOURCE_DIR}/googletest" "googletest")

include_directories(
    ../src
)

set (PROJECT_SRCS
    ${PROJECT_SOURCE_DIR}/src/chip8_pc.cpp
    ${PROJECT_SOURCE_DIR}/src/chip8_pc.h
    ${PROJECT_SOURCE_DIR}/src/monitor.cpp
    ${PROJECT_SOURCE_DIR}/src/monitor.h
)


set(TESTNAME BasicTests)

add_executable(${TESTNAME} test.cpp ${PROJECT_SRCS})
target_link_libraries(${TESTNAME} PRIVATE gtest gmock gtest_main)
target_link_libraries(${TESTNAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
target_link_libraries(${TESTNAME} PRIVATE fmt::fmt)

gtest_discover_tests(${TESTNAME}
    # set a working directory so your project root so that you can find test data via paths relative to the project root
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
)

set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
