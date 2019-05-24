# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
list(APPEND QEMU_EXTRA_FLAGS -serial tcp:localhost:8888)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(nfc_hello)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
