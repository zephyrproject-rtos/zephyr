# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(NOT("${SB_CONFIG_NET_CORE_BOARD}" STREQUAL ""))
	set(NET_APP hci_ipc)
	set(NET_APP_SRC_DIR ${ZEPHYR_BASE}/samples/bluetooth/${NET_APP})

	ExternalZephyrProject_Add(
		APPLICATION ${NET_APP}
		SOURCE_DIR  ${NET_APP_SRC_DIR}
		BOARD       ${SB_CONFIG_NET_CORE_BOARD}
	)

	set(${NET_APP}_CONF_FILE ${APP_DIR}/nrf5340_cpunet-bt_ll_sw_split.conf CACHE INTERNAL "")

	native_simulator_set_primary_mcu_index(${DEFAULT_IMAGE} ${NET_APP})
	native_simulator_set_child_images(${DEFAULT_IMAGE} ${NET_APP})
endif()
native_simulator_set_final_executable(${DEFAULT_IMAGE})
