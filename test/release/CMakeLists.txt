cmake_minimum_required(VERSION 3.10)
project(Test)

find_package(catui REQUIRED)

add_executable(test test.c)
target_link_libraries(test PRIVATE catui)
