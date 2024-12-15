# Minimum CMake version
cmake_minimum_required(VERSION 3.10)


project(CoconutPluginProject LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_program(GCC_COMPILER g++-13)
if(NOT GCC_COMPILER)
    message(FATAL_ERROR "GCC 13 (g++-13) not found. Please install GCC 13 and make it accessible.")
endif()


set(CMAKE_CXX_COMPILER ${GCC_COMPILER})


execute_process(COMMAND ${GCC_COMPILER} -print-file-name=plugin
                OUTPUT_VARIABLE GCC_PLUGIN_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "GCC Plugin Directory: ${GCC_PLUGIN_DIR}")

# Include paths for GCC plugin headers
include_directories(
    ${GCC_PLUGIN_DIR}/include
    ${GCC_PLUGIN_DIR}/include/gcc
    ${GCC_PLUGIN_DIR}/include/gcc/cp
    ${GCC_PLUGIN_DIR}/include/gcc/c-family
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# Optimization flags based on build type
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0") # Debugging symbols, no optimization
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -DNDEBUG") # High optimization
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -DNDEBUG") # Optimized build with debug info
endif()

# Define source files
set(PLUGIN_NAME Coconut)
set(PLUGIN_SRC src/Coconut.cpp)

set(Robot_EXE robot)
set(Robot Robot/main.cpp)

set(Robot_OP robotOP)
set(RobotOP Robot/OPmain.cpp)

set(Client12_EXE Http-connection_)
set(Client12 Http-connection/main.cpp)

set(Client1_EXE Pillbox_)
set(Client1 PillBox/main.cpp)

set(Client11_EXE LightSwitch_)
set(Client11 LightSwitch_Example/main.cpp)

set(Client2_EXE Sound_)
set(Client2 Sound/main.cpp)

set(Client3_EXE Basket_)
set(Client3 Basket/main.cpp)

set(Client4_EXE File_)
set(Client4 File_example/main.cpp)

set(Client5_EXE Student_)
set(Client5 Student/main.cpp)

set(Client6_EXE Case2_)
set(Client6 Inheritance/Case_2/main.cpp)

set(Client7_EXE Case3_)
set(Client7 Inheritance/Case_3/main.cpp)

set(Client8_EXE Case4_)
set(Client8 Inheritance/Case_4/main.cpp)

set(Client9_EXE Case4C_)
set(Client9 Inheritance/Case_4/Cmain.cpp)






# Add shared library for Coconut plugin
add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SRC})
set_target_properties(${PLUGIN_NAME} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    OUTPUT_NAME "Coconut"
    INSTALL_RPATH "$ORIGIN"
)

# Add executables
add_executable(${Client12_EXE} ${Client12})
add_executable(${Client1_EXE} ${Client1})
add_executable(${Client11_EXE} ${Client11})
add_executable(${Client2_EXE} ${Client2})
add_executable(${Client3_EXE} ${Client3})
add_executable(${Client4_EXE} ${Client4})
add_executable(${Client5_EXE} ${Client5})
add_executable(${Client6_EXE} ${Client6})
add_executable(${Client7_EXE} ${Client7})
add_executable(${Client8_EXE} ${Client8})
add_executable(${Client9_EXE} ${Client9})
add_executable(${Robot_EXE} ${Robot})
add_executable(${Robot_OP} ${RobotOP})

# Use the plugin during compilation
target_compile_options(${Client12_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client1_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client11_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client2_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client4_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client5_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client6_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client7_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client8_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client9_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Client3_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Robot_EXE} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
target_compile_options(${Robot_OP} PRIVATE -fplugin=$<TARGET_FILE:${PLUGIN_NAME}>)
