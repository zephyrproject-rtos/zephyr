# SPDX-License-Identifier: Apache-2.0

# Clear the toolchain defined C++ library property when using minimal C++ lib.
set_linker_property(PROPERTY c++_library  "")
set_linker_property(PROPERTY cpp_base "")

zephyr_system_include_directories(include)

zephyr_sources(
  cpp_virtual.c
  cpp_new.cpp
)

# The IAR compiler reserves the __cxxabiv1 type_info class names, the vtables
# are provided by the IAR C++ runtime library.
if(NOT ZEPHYR_TOOLCHAIN_VARIANT STREQUAL "iar")
  zephyr_sources(cpp_vtable.cpp)
endif()
