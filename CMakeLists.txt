cmake_minimum_required(VERSION 3.5)
project(stress)
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} STATIC stress.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC StressLib)

include_directories(${PROJECT_SOURCE_DIR}/include)
find_package(Threads)
target_link_libraries(${PROJECT_NAME} Threads::Threads)