# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_EXTERNAL_LIBC)
  zephyr_syscall_header(
    ${ZEPHYR_BASE}/include/zephyr/sys/libc-hooks.h
    )

  add_subdirectory_ifdef(CONFIG_ARCMWDT_LIBC      arcmwdt)
  add_subdirectory_ifdef(CONFIG_ARMCLANG_STD_LIBC armstdc)
  add_subdirectory_ifdef(CONFIG_IAR_LIBC          iar)
  add_subdirectory_ifdef(CONFIG_MINIMAL_LIBC      minimal)
  add_subdirectory_ifdef(CONFIG_NEWLIB_LIBC       newlib)
  add_subdirectory_ifdef(CONFIG_PICOLIBC          picolibc)

  add_subdirectory(common)
endif()

zephyr_sources(
  validate_libc.c
  )
