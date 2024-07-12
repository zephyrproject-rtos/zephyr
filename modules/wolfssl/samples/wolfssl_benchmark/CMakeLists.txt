cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wolfssl_benchmark)

target_sources(app PRIVATE ${ZEPHYR_WOLFSSL_MODULE_DIR}/wolfcrypt/benchmark/benchmark.c)
target_include_directories(app PRIVATE ${ZEPHYR_WOLFSSL_MODULE_DIR}/wolfcrypt/benchmark)
target_sources(app PRIVATE ${app_sources})
add_definitions(-DWOLFSSL_USER_SETTINGS)
