# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/mspi.h)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MSPI_AMBIQ_AP3   mspi_ambiq_ap3.c)
zephyr_library_sources_ifdef(CONFIG_MSPI_AMBIQ_AP5   mspi_ambiq_ap5.c)
zephyr_library_sources_ifdef(CONFIG_MSPI_DW          mspi_dw.c)
zephyr_library_sources_ifdef(CONFIG_MSPI_EMUL        mspi_emul.c)
zephyr_library_sources_ifdef(CONFIG_MSPI_ESP32       mspi_esp32.c)

zephyr_sources_ifdef(
    CONFIG_MSPI_ESP32
    ${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR}/components/hal/spi_hal.c
    ${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR}/components/hal/spi_hal_iram.c
    ${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR}/components/soc/lldesc.c
    ${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR}/components/hal/gdma_hal.c
)