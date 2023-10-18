cmake_minimum_required(VERSION 3.16.3)

project(COCONUT)
#Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost setup
set(Boost_ROOT "C:/Program Files/boost_1_81_0") # put the right path to your boost file 
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})


# Add the project's include directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add executable targets for each client
add_executable(Client1 PillBox/main.cpp)
add_executable(Client2 File_example/main.cpp)
add_executable(Client3 BankAccount_Example/main.cpp)
add_executable(Client4 Sound/main.cpp)
add_executable(Client5 Student/main.cpp)
add_executable(Client6 Basket/main.cpp)
add_executable(Client7 Inheritance/Case_1/main.cpp)
add_executable(Client8 Inheritance/Case_2/main.cpp)
add_executable(Client9 Inheritance/Case_3/main.cpp)
add_executable(Client10 Inheritance/Case_4/main.cpp)
# Include Boost for all executable targets
target_include_directories(Client1 PRIVATE Boost)
target_include_directories(Client2 PRIVATE Boost)
target_include_directories(Client3 PRIVATE Boost)
target_include_directories(Client4 PRIVATE Boost)
target_include_directories(Client5 PRIVATE Boost)
target_include_directories(Client6 PRIVATE Boost)
target_include_directories(Client8 PRIVATE Boost)
target_include_directories(Client9 PRIVATE Boost)
target_include_directories(Client10 PRIVATE Boost)
# Include TypestateLibrary.h for all executable targets
target_include_directories(Client1 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client2 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client3 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client4 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client5 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client6 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client8 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client9 PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
target_include_directories(Client10 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/include/Typestate_Library/ ${CMAKE_CURRENT_SOURCE_DIR}/external/)
