cmake_minimum_required(VERSION 3.16)
project(search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 参考
# add_executable(search_server src/app/search_server.cpp)
# target_include_directories(search_server PRIVATE ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(search_server PRIVATE muduo_net muduo_base pthread)

#
# add_executable(build_dictionary src/app/build_dictionary.cpp)
# add_executable(build_webpage_index src/app/build_webpage_index.cpp)
