cmake_minimum_required(VERSION 3.16)
project(search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../)


find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
if (NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(FATAL_ERROR "缺少 nlohmann/json.hpp")
endif ()

# 参考
# add_executable(search_server src/app/search_server.cpp)
# target_include_directories(search_server PRIVATE ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(search_server PRIVATE muduo_net muduo_base pthread)

#
# add_executable(build_offline_data src/app/build_offline_data.cpp)
# add_executable(build_webpage_index src/app/build_webpage_index.cpp)

add_library(env_loader STATIC src/core/EnvLoader.cpp)
target_include_directories(env_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(dir_scaner STATIC src/core/DirectoryScanner.cpp)
target_include_directories(dir_scaner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(keyword_proc STATIC src/recommend/KeywordProcessor.cpp)
target_include_directories(keyword_proc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(page_proc STATIC src/webpage/PageProcessor.cpp)
target_link_libraries(page_proc PRIVATE tinyxml2)
target_include_directories(page_proc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(web_search STATIC
        src/webpage/DocLibrary.cpp
        src/webpage/InvertedIndex.cpp
        src/webpage/WebSearcher.cpp)
target_link_libraries(web_search PRIVATE env_loader tinyxml2)
target_include_directories(web_search
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})

add_executable(build_offline_data src/build_offline_data.cpp)
target_link_libraries(build_offline_data PRIVATE env_loader keyword_proc dir_scaner page_proc)
