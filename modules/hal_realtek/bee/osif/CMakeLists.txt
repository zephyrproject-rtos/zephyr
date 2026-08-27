# Copyright(c) 2026, Realtek Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(common)
zephyr_library_sources(common/osif_zephyr_impl.c)

set(OSIF_INC_ROOT "${ZEPHYR_HAL_REALTEK_MODULE_DIR}/bee/os/osif/inc")

if(CONFIG_SOC_SERIES_RTL87X2G)
  zephyr_include_directories(${OSIF_INC_ROOT}/rtl87x2g)
  zephyr_library_sources(rtl87x2g/osif_zephyr_patch.c)

elseif(CONFIG_SOC_SERIES_RTL8752H)
  zephyr_include_directories(${OSIF_INC_ROOT}/rtl8752h)
  zephyr_library_sources(rtl8752h/osif_zephyr_patch.c)

endif()
