# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  libc-hooks.c
  multithreading.c
)

zephyr_system_include_directories(include)

zephyr_compile_definitions(
  _BLKCNT_T_DECLARED
  _BLKSIZE_T_DECLARED
  _DEV_T_DECLARED
  _INO_T_DECLARED
)
