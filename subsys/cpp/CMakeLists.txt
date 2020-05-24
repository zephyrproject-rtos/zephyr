# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  cpp_init_array.c
  cpp_ctors.c
  cpp_dtors.c
)

if (NOT CONFIG_LIB_CPLUSPLUS AND
        (NOT CONFIG_MINIMAL_LIBC OR
        (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE GREATER 0)))
zephyr_sources(
  cpp_virtual.c
  cpp_vtable.cpp
  cpp_new.cpp
)
endif()

if(CONFIG_STD_THREAD)
  if(NOT (CONFIG_BOARD_NATIVE_POSIX OR CONFIG_BOARD_NATIVE_POSIX_64BIT OR CONFIG_BOARD_NRF52_BSIM))
    #zephyr_compile_options(
    #  $<$<COMPILE_LANGUAGE:CXX>:-include${PROJECT_SOURCE_DIR}/include/bits/gthr-zephyr.h>
    #)
    zephyr_sources(std_thread.cpp)
  endif()
endif()
