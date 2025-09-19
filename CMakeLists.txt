cmake_minimum_required(VERSION 3.10)


find_program(GXX_13 g++-13 REQUIRED)
find_program(GCC_13 gcc-13 REQUIRED)
set(CMAKE_C_COMPILER   "${GCC_13}")
set(CMAKE_CXX_COMPILER "${GXX_13}")

project(CoconutPluginProject LANGUAGES C CXX)


# Language / flags

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GCC plugin includes 

execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=plugin
  OUTPUT_VARIABLE GCC_PLUGIN_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "GCC Plugin Directory: ${GCC_PLUGIN_DIR}")

include_directories(
  ${GCC_PLUGIN_DIR}/include
  ${GCC_PLUGIN_DIR}/include/gcc
  ${GCC_PLUGIN_DIR}/include/gcc/cp
  ${GCC_PLUGIN_DIR}/include/gcc/c-family
)

# Build plugins

# Analysis plugin -> libCoconut.so
add_library(Coconut SHARED src/Coconut.cpp)
set_target_properties(Coconut PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  OUTPUT_NAME "Coconut"
  INSTALL_RPATH "$ORIGIN"
)
# Let unresolved GCC internals resolve at load time in cc1/cc1plus
set_property(TARGET Coconut APPEND_STRING PROPERTY LINK_FLAGS
  " -Wl,--unresolved-symbols=ignore-in-shared-libs -Wl,-z,undefs")

# Runtime plugin -> libCoconutRT.so
add_library(CoconutRT SHARED src/Coconut-RT.cpp)
set_target_properties(CoconutRT PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  OUTPUT_NAME "CoconutRT"
  INSTALL_RPATH "$ORIGIN"
)
set_property(TARGET CoconutRT APPEND_STRING PROPERTY LINK_FLAGS
  " -Wl,--unresolved-symbols=ignore-in-shared-libs -Wl,-z,undefs")

# Space-safe symlinks for -fplugin
add_custom_command(TARGET Coconut POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:Coconut> /tmp/libCoconut.so
  VERBATIM)
add_custom_command(TARGET CoconutRT POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:CoconutRT> /tmp/libCoconutRT.so
  VERBATIM)


# ts_runtime: prefer prebuilt object, else build from C

set(TS_OBJ "${CMAKE_SOURCE_DIR}/src/ts_runtime.o")
set(TS_LINK "")
if(EXISTS "${TS_OBJ}")
  message(STATUS "Using prebuilt runtime object: ${TS_OBJ}")
  set(TS_LINK "$<SHELL_PATH:${TS_OBJ}>")
else()
  message(STATUS "Building ts_runtime from src/ts_runtime.c")
  add_library(ts_runtime STATIC src/ts_runtime.c)
  set_target_properties(ts_runtime PROPERTIES LINKER_LANGUAGE C)
endif()

# Tiny bootstrap that loads TS_RULES_PATH at startup (see src/ts_boot.c below)
add_library(ts_boot STATIC src/ts_boot.c)
set_target_properties(ts_boot PROPERTIES LINKER_LANGUAGE C)


#  executables

set(Robot_EXE    robot)
set(Robot        Robot/main.cpp)

set(Robot_OP     robotOP)
set(RobotOP      Robot/OPmain.cpp)

set(Client12_EXE Http-connection_)
set(Client12     Http-connection/main.cpp)

set(Client1_EXE  Pillbox_)
set(Client1      PillBox/main.cpp)

set(Client11_EXE LightSwitch_)
set(Client11     LightSwitch_Example/main.cpp)

set(Client2_EXE  Sound_)
set(Client2      Sound/main.cpp)

set(Client3_EXE  Basket_)
set(Client3      Basket/main.cpp)

set(Client4_EXE  File_)
set(Client4      File_example/main.cpp)

set(Client5_EXE  Student_)
set(Client5      Student/main.cpp)

set(Client6_EXE  Case2_)
set(Client6      Inheritance/Case_2/main.cpp)

set(Client7_EXE  Case3_)
set(Client7      Inheritance/Case_3/main.cpp)

set(Client8_EXE  Case4_)
set(Client8      Inheritance/Case_4/main.cpp)

set(Client9_EXE  Case4C_)
set(Client9      Inheritance/Case_4/Cmain.cpp)

set(Robot_may    robotmay)
set(Robotmay     Robot/may.cpp)

set(Robot_th     robotth)
set(Robotth      Robot/threads.cpp)

add_executable(${Client12_EXE} ${Client12})
add_executable(${Client1_EXE}  ${Client1})
add_executable(${Client11_EXE} ${Client11})
add_executable(${Client2_EXE}  ${Client2})
add_executable(${Client3_EXE}  ${Client3})
add_executable(${Client4_EXE}  ${Client4})
add_executable(${Client5_EXE}  ${Client5})
add_executable(${Client6_EXE}  ${Client6})
add_executable(${Client7_EXE}  ${Client7})
add_executable(${Client8_EXE}  ${Client8})
add_executable(${Client9_EXE}  ${Client9})
add_executable(${Robot_EXE}    ${Robot})
add_executable(${Robot_OP}     ${RobotOP})
add_executable(${Robot_may}    ${Robotmay})
add_executable(${Robot_th}     ${Robotth})

# Attach analysis plugin (Coconut) to the “normal” executables
function(use_coconut_plugin tgt)
  add_dependencies(${tgt} Coconut)
  target_compile_options(${tgt} BEFORE PRIVATE "SHELL:-fplugin=/tmp/libCoconut.so")
endfunction()

use_coconut_plugin(${Client12_EXE})
use_coconut_plugin(${Client1_EXE})
use_coconut_plugin(${Client11_EXE})
use_coconut_plugin(${Client2_EXE})
use_coconut_plugin(${Client3_EXE})
use_coconut_plugin(${Client4_EXE})
use_coconut_plugin(${Client5_EXE})
use_coconut_plugin(${Client6_EXE})
use_coconut_plugin(${Client7_EXE})
use_coconut_plugin(${Client8_EXE})
use_coconut_plugin(${Client9_EXE})
use_coconut_plugin(${Robot_EXE})
use_coconut_plugin(${Robot_OP})
use_coconut_plugin(${Robot_th})


# Where analyzer writes rules; runtime reads from here
set(MARK_DIR   "${CMAKE_BINARY_DIR}/needs_rt")
set(RULES_FILE "${MARK_DIR}/robotmay.rules")
set(MARK_FILE  "${MARK_DIR}/robotmay.mark")

# Prepare output dir and clean previous outputs
add_custom_target(_prep_rules ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<SHELL_PATH:${MARK_DIR}>"
  COMMAND ${CMAKE_COMMAND} -E rm -f "$<SHELL_PATH:${RULES_FILE}>" "$<SHELL_PATH:${MARK_FILE}>"
  VERBATIM)

# Run the ANALYSIS plugin on Robot/may.cpp to generate rules (no link)
# Prefix MUST match filename 'libCoconut.so' → 'libCoconut'
add_custom_target(emit_rules ALL
  COMMAND ${CMAKE_CXX_COMPILER}
          -fplugin=/tmp/libCoconut.so
          -fplugin-arg-libCoconut-mark=$<SHELL_PATH:${MARK_FILE}>
          -fplugin-arg-libCoconut-rules-out=$<SHELL_PATH:${RULES_FILE}>
          -S -x c++ $<SHELL_PATH:${CMAKE_SOURCE_DIR}/Robot/may.cpp>
          -o /dev/null
  DEPENDS _prep_rules Coconut
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM)


add_executable(may Robot/may.cpp)
add_executable(roboth Robot/Threads.cpp)
#add_dependencies(may CoconutRT emit_rules)
 target_compile_options(may  PRIVATE "SHELL:-fplugin=/tmp/libCoconutRT.so" )
target_link_libraries(may PRIVATE "$<SHELL_PATH:${TS_OBJ}>")

 target_compile_options(roboth  PRIVATE "SHELL:-fplugin=/tmp/libCoconutRT.so" )
target_link_libraries(roboth PRIVATE "$<SHELL_PATH:${TS_OBJ}>")
