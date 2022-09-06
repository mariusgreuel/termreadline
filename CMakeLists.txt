cmake_minimum_required(VERSION 3.12)

project(main)

find_package(unofficial-readline-win32 CONFIG REQUIRED)
add_executable(main termreadline.c "msvc/usleep.cpp")
target_link_libraries(main PRIVATE unofficial::readline-win32::readline)
