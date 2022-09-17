cmake_minimum_required(VERSION 3.12)

project(main)
add_executable(main termreadline.c)

if (MSVC)
    find_package(unofficial-readline-win32 CONFIG REQUIRED)
    target_sources(main PRIVATE "msvc/usleep.cpp")
    target_link_libraries(main PRIVATE unofficial::readline-win32::readline)
else()
    target_link_libraries(main PRIVATE readline)
endif()
