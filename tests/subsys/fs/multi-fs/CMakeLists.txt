cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

zephyr_compile_definitions(
  -DTEST_FLASH_OFFSET=0
  -DFLASH_AREA_STORAGE_OFFSET=0
  -DFLASH_AREA_STORAGE_SIZE=1048576
)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
