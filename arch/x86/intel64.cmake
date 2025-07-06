# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_cc_option(-m64)
zephyr_cc_option(-mno-red-zone)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH "i386:x86-64")
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf64-x86-64")
get_property(OUTPUT_ARCH   GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH)
get_property(OUTPUT_FORMAT GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

if(CONFIG_X86_SSE)
  # x86-64 by default has SSE and SSE2
  # so no need to add compiler flags for them.

  if(CONFIG_X86_SSE3)
    zephyr_cc_option(-msse3)
  else()
    zephyr_cc_option(-mno-sse3)
  endif()

  if(CONFIG_X86_SSSE3)
    zephyr_cc_option(-mssse3)
  else()
    zephyr_cc_option(-mno-ssse3)
  endif()

  if(CONFIG_X86_SSE41)
    zephyr_cc_option(-msse4.1)
  else()
    zephyr_cc_option(-mno-sse4.1)
  endif()

  if(CONFIG_X86_SSE42)
    zephyr_cc_option(-msse4.2)
  else()
    zephyr_cc_option(-mno-sse4.2)
  endif()

  if(CONFIG_X86_SSE4A)
    zephyr_cc_option(-msse4a)
  else()
    zephyr_cc_option(-mno-sse4a)
  endif()

endif()

add_subdirectory(core)
