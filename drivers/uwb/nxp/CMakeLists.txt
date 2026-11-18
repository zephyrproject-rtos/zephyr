# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
    uwb_bus_interface.c
    uwb_bus_io.c
    uwb_nxp_host.c
    uwb_tml_transport.c
)

zephyr_include_directories(
    .
)

zephyr_compile_definitions(
    -DUWBIOT_USE_FTR_FILE
)

add_subdirectory_ifdef(CONFIG_SHIELD_NXP_SR250 sr250)

zephyr_include_directories_ifdef(
    CONFIG_SHIELD_NXP_SR250
    ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/sr250
)
