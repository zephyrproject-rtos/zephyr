cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(benchmark)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_compile_definitions(
    -DNO_PROTOTYPES=1
    -DHZ=100
)

zephyr_compile_options(
    -mfloat-abi=soft
    -Wno-implicit-int
    -Wno-implicit-function-declaration
    -Wno-return-type
    -Wno-maybe-uninitialized
)
