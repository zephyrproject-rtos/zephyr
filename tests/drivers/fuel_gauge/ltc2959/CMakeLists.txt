cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_ltc2959)

target_sources(app PRIVATE test_ltc2959.c)
