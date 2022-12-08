# SPDX-License-Identifier: Apache-2.0

add_subdirectory(abi)

if (NOT CONFIG_LIB_CPLUSPLUS)
zephyr_system_include_directories(
	include
)
endif()

if (NOT CONFIG_LIB_CPLUSPLUS AND
        (NOT CONFIG_MINIMAL_LIBC OR
        (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE GREATER 0)))
zephyr_sources(
  cpp_virtual.c
  cpp_vtable.cpp
  cpp_new.cpp
)
endif()
