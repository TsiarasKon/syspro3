cmake_minimum_required(VERSION 3.8)
project(myhttpd)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c ../common/util.c ../common/requests.c ../common/lists.c ../common/util.h ../common/requests.h ../common/lists.h)
add_executable(myhttpd ${SOURCE_FILES})

target_link_libraries(myhttpd pthread)