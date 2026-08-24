#
# Copyright 2025 NXP
# Copyright (c) 2026 CogniPilot Foundation
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
# board.c provides board_early_init_hook, selected via BOARD_EARLY_INIT_HOOK and
# always called by the kernel. Keep it outside the boot-header block below so it
# is compiled even when the image carries no boot header of its own.
zephyr_library_sources(board.c)

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  if(NOT DEFINED CONFIG_BOARD_MR_VMU_TROPIC)
    message(WARNING "It appears you are using the board definition for "
     "the MR-VMU-TROPIC, but targeting a custom board. You may need to "
     "update your flash configuration or device configuration data blocks")
  endif()
  if(CONFIG_BOOT_FLEXSPI_NOR)
    # This flash configuration block may need modification if another
    # flash chip is used on your custom board. See NXP AN12238 for more
    # information.
    zephyr_library_sources(xip/mr_vmu_tropic_flexspi_nor_config.c)
    zephyr_library_include_directories(xip)
  endif()
endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
