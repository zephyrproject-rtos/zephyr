cmake_minimum_required(VERSION 3.7)

add_subdirectory($ENV{ZEPHYR_BASE} ${CMAKE_CURRENT_BINARY_DIR}/zephyr)
target_sources(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
