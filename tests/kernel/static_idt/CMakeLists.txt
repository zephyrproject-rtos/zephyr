cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language(C ASM)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources} src/test_stubs.S)
