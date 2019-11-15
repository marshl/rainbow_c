cmake_minimum_required(VERSION 3.15)
project(rainbow_c)

set(CMAKE_CXX_STANDARD 14)

add_executable(rainbow_c main.cpp bmp.h colour.h point.h pixel.h rainbow_renderer.h)