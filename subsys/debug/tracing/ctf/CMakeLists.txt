# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ctf_top.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_include_directories(.)

add_subdirectory_ifdef(CONFIG_TRACING_CTF_BOTTOM_POSIX bottoms/posix)
