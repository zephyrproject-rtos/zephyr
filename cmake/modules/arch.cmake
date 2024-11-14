# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023, Nordic Semiconductor ASA

#
# Configure ARCH settings based on KConfig settings and arch root.
#
# This CMake module will set the following variables in the build system based
# on board directory and arch root.
#
# If no implementation is available for the current arch an error will be raised.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - ARCH:      Name of the arch in use.
# - ARCH_DIR:  Directory containing the arch implementation.
# - ARCH_ROOT: ARCH_ROOT with ZEPHYR_BASE appended
#
# Variable dependencies:
# - ARCH_ROOT: CMake list of arch roots containing arch implementations
#
# Variables set by this module and not mentioned above are considered internal
# use only and may be removed, renamed, or re-purposed without prior notice.

include_guard(GLOBAL)

if(HWMv2)
  # HWMv2 obtains arch from Kconfig for the given Board / SoC variant because
  # the Board / SoC path is no longer sufficient for determine the arch
  # (read: multi-core and multi-arch SoC).
  set(ARCH ${CONFIG_ARCH})
  string(TOUPPER "${ARCH}" arch_upper)

  if(NOT ARCH)
    message(FATAL_ERROR "ARCH not defined. Check that BOARD=${BOARD}, is selecting "
            "an appropriate SoC in Kconfig, SoC=${CONFIG_SOC}, and that the SoC "
            "is selecting the correct architecture."
    )
  endif()

  cmake_path(GET ARCH_V2_${arch_upper}_DIR PARENT_PATH ARCH_DIR)
  if(NOT ARCH_DIR)
    message(FATAL_ERROR "Could not find ARCH=${ARCH} for BOARD=${BOARD}, \
please check your installation. ARCH roots searched: \n\
${ARCH_ROOT}")
  endif()
endif()
