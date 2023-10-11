# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(QEMU_EXTRA_FLAGS -s)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh)

target_sources(app PRIVATE src/main.c)

if (BOARD STREQUAL bbc_microbit)
  target_sources(app PRIVATE src/microbit.c)
else()
  target_sources(app PRIVATE src/board.c)
endif()

if (CONFIG_BUILD_WITH_TFM)
  target_include_directories(app PRIVATE
    $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/install/interface/include
  )
endif()
