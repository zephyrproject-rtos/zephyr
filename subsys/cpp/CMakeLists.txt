# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CPLUSPLUS
  cpp_virtual.c
  cpp_vtable.cpp
  cpp_init_array.c
  cpp_ctors.c
  cpp_dtors.c
  cpp_new.cpp
)
