# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  cpp_init_array.c
  cpp_ctors.c
  cpp_dtors.c
)

if (NOT CONFIG_LIB_CPLUSPLUS OR CONFIG_ZEPHYR_CPLUSPLUS)
zephyr_sources(
  cpp_virtual.c
  cpp_vtable.cpp
  cpp_new.cpp
)
endif()
