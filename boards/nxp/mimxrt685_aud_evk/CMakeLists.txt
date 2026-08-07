# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_MIMXRT685_AUD_EVK_MIMXRT685S_CM33)
  zephyr_library()
  zephyr_library_sources(cm33/init.c)
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  if(NOT DEFINED CONFIG_BOARD_MIMXRT685_AUD_EVK_MIMXRT685S_CM33)
    message(WARNING "It appears you are using the board definition for "
      "the MIMXRT685-AUD-EVK, but targeting a custom board. You may need to "
      "update your flash configuration block data")
  endif()

  zephyr_library_sources(flash_config/flash_config.c)
  zephyr_library_include_directories(flash_config)
endif()
