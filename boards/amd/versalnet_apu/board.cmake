#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)

# Set TF-A platform for ARM Trusted Firmware builds
if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "versal_net")
  # Configure TF-A memory location for Versal NET platform
  # TF-A runs from DDR at address 0xf000000 (changed from default 0xbbf00000).
  # This DDR location (VERSAL_NET_ATF_MEM_BASE=0xf000000) works for all possible designs.
  # Note: If TF-A needs to run from OCM instead of DDR, PDI changes would be required.
  set(TFA_EXTRA_ARGS "RESET_TO_BL31=1;PRELOADED_BL33_BASE=0x0;TFA_NO_PM=1;VERSAL_NET_ATF_MEM_BASE=0xf000000;VERSAL_NET_ATF_MEM_SIZE=0x50000")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
endif()
