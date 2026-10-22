# SPDX-License-Identifier: Apache-2.0

# Clear the toolchain defined C++ library property when using minimal C++ lib.
set_linker_property(PROPERTY c++_library  "")
set_linker_property(PROPERTY cpp_base "")

zephyr_system_include_directories(include)

zephyr_sources(
  cpp_virtual.c
  cpp_vtable.cpp
  cpp_new.cpp
)
