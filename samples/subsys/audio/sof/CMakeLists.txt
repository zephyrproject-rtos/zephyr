# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(sample_sof)

target_sources(app PRIVATE
    src/main.c
  )

zephyr_interface_library_named(sof_lib)

zephyr_library_include_directories(app PUBLIC
    ${sof_module}/src/arch/xtensa/include
    ${sof_module}/src/include
  )
