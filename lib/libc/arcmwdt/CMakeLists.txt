# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

zephyr_library()
zephyr_library_sources(
  arcmwdt-string.c
  libc-hooks.c
  threading.c
)
