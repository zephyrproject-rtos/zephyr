# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026  Gaiaochos

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(mcuboot PROPERTIES BOARD nucleo_n657x0_q/stm32n657xx/fsbl)
endif()
