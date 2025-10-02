#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library_compile_definitions(BOOT_HEADER_ENABLE=1)
  zephyr_library_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
endif()

if(CONFIG_SOC_MIMXRT595S_CM33)
  add_subdirectory(cm33)
elseif(CONFIG_SOC_MIMXRT595S_F1)
  add_subdirectory(f1)
endif()
