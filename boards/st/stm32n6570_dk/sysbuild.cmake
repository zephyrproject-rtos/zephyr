# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2025 STMicroelectronics

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(mcuboot PROPERTIES BOARD stm32n6570_dk/stm32n657xx/fsbl)
endif()
