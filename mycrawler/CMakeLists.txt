cmake_minimum_required(VERSION 3.8)
project(mycrawler)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c util.c requests.c lists.c lists.h)
add_executable(mycrawler ${SOURCE_FILES})

target_link_libraries(mycrawler pthread)