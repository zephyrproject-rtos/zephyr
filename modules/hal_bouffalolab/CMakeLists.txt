# Copyright (c) 2021-2024 Gerson Fernando Budke <nandojve@gmail.com>
# Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_BFLB)

if (CONFIG_SOC_SERIES_BL60X)
  zephyr_compile_definitions(BL602)
endif()

zephyr_include_directories(${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES})
zephyr_include_directories(${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include)

endif()
