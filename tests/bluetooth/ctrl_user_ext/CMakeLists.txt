# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include_directories("./src")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(bluetooth_ctrl_user_ext)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})
