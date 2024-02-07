project(COCONUT)
# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Boost setup

find_package(Graphviz)
find_package(Boost)
include_directories(${GRAPHVIZ_INCLUDE_DIR})

# Define a list of all client source files and their corresponding target names
set(CLIENT_SOURCES_AND_NAMES
    PillBox/main.cpp Client1
    File_example/main.cpp Client2
    #BankAccount_Example/main.cpp Client3
    Sound/main.cpp Client4
    Student/main.cpp Client5
    Basket/main.cpp Client6
    Inheritance/Case_1/main.cpp Client7
    Inheritance/Case_2/main.cpp Client8
    Inheritance/Case_3/main.cpp Client9
    Inheritance/Case_4/main.cpp Client10
    LightSwitch_Example/main.cpp Client11
    Http_connection/main.cpp Client12
)

# Use list(LENGTH) and math(EXPR) to iterate through the list in pairs
list(LENGTH CLIENT_SOURCES_AND_NAMES num_items)
math(EXPR num_pairs "${num_items} / 2 - 1")

foreach(pair RANGE ${num_pairs})
    math(EXPR source_idx "${pair} * 2")
    math(EXPR name_idx "${source_idx} + 1")
    list(GET CLIENT_SOURCES_AND_NAMES ${source_idx} source_file)
    list(GET CLIENT_SOURCES_AND_NAMES ${name_idx} target_name)

    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE 
        ${Boost_INCLUDE_DIRS} ${GRAPHVIZ_INCLUDE_DIR} ${GRAPHVIZ_LIBRARY}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Typestate_Library 
        ${CMAKE_CURRENT_SOURCE_DIR}/external
    )
endforeach()
