# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

zephyr_sources(
  cpp_virtual.c
  cpp_vtable.cpp
  cpp_new.cpp
)
