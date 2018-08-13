cmake_minimum_required(VERSION 3.8.2)
# Re-direct the directory where the 'boards' directory is found from
# $ZEPHYR_BASE to this directory.
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

# This sample is only compatible with nrf52840_pca10065 because it has
# re-directed the 'boards' directory to a directory that only contains
# this board.
set(BOARD nrf52840_pca10056)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)
