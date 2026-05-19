# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(mcuboot PROPERTIES BOARD dnn647/stm32n647xx/fsbl)
endif()
