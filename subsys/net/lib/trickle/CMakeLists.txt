# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

zephyr_library()

zephyr_library_sources(
  trickle.c
  )
