# Copyright (c) 2022 Cypress Semiconductor Corporation.
#
# SPDX-License-Identifier: Apache-2.0

set(bless_dir            ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/bless)
set(bless_blobs_dir      ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr/blobs/img/bluetooth/firmware)

zephyr_include_directories(${bless_dir}/include)

zephyr_compile_definitions(COMPONENT_BLESS_CONTROLLER)

zephyr_library_sources(${bless_dir}/cy_ble.c)
zephyr_library_sources(${bless_dir}/cy_ble_clk.c)
zephyr_library_sources(${bless_dir}/cy_ble_common.c)
zephyr_library_sources(${bless_dir}/cy_ble_controller.c)
zephyr_library_sources(${bless_dir}/cy_ble_event_handler.c)
zephyr_library_sources(${bless_dir}/cy_ble_hal_int.c)
zephyr_library_sources(${bless_dir}/cy_ble_hal_pvt.c)

zephyr_link_libraries(
	${bless_blobs_dir}/COMPONENT_BLESS_CONTROLLER/cy_ble_stack_controller.a
	${bless_blobs_dir}/COMPONENT_BLESS_CONTROLLER/cy_ble_stack_manager.a
)
