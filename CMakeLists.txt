cmake_minimum_required(VERSION 3.15)
project(rainbow_c)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(rainbow_c main.cpp stb_image_write.h colour.h point.h pixel.h rainbow_renderer.h
        rainbow_renderer.cpp colour.cpp thread_pool.h thread_pool.cpp)

target_link_libraries(rainbow_c PRIVATE Threads::Threads)