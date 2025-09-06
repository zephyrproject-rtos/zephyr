#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_NXP_MCXE31X_BOOT_HEADER)
  if(NOT ((DEFINED CONFIG_BOARD_FRDM_MCXE31B_MCXE31B)))
    message(WARNING "It appears you are using the board definition for "
     "the MIMXRT1180-EVK, but targeting a custom board. You may need to "
     "update your flash configuration or device configuration data blocks")
  endif()
    # This flash configuration block may need modification if another
    # flash chip is used on your custom board.
    zephyr_compile_definitions(BOOT_HEADER_ENABLE=1)
    zephyr_library_sources(boot_header/boot_header.c)
    zephyr_library_include_directories(boot_header)
endif()
