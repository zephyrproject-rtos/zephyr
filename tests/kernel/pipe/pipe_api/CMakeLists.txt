cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)

target_sources(app
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/basic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/stress.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/concurrency.c
)
