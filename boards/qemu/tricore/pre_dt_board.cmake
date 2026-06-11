# Copyright (c) 2026 Infineon Technologies AG
#
# SPDX-License-Identifier: Apache-2.0
#
# TriCore is not yet supported by the Zephyr SDK (see sdk-ng#1139).
# When upstream CI runs with ZEPHYR_TOOLCHAIN_VARIANT=zephyr, redirect to
# cross-compile and use the externally-provided tricore-elf toolchain
# from the docker-image (see docker-image PR #301).
#
# Once sdk-ng#1139 lands and a new Zephyr SDK ships with TriCore support,
# this file can be removed.

if("${ZEPHYR_TOOLCHAIN_VARIANT}" MATCHES "^zephyr/?")
  set(ZEPHYR_TOOLCHAIN_VARIANT "cross-compile" CACHE STRING
      "Zephyr toolchain variant" FORCE)
  if(NOT DEFINED ENV{CROSS_COMPILE} AND NOT DEFINED CROSS_COMPILE)
    set(ENV{CROSS_COMPILE} "/opt/toolchains/tricore-elf/bin/tricore-elf-")
  endif()
endif()
