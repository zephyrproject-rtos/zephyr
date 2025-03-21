# Copyright (c) 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_NET_CORE_IMAGE_HCI_IPC)
	# For builds in the nrf5340, we build the netcore image with the controller

	set(NET_APP hci_ipc)
	set(NET_APP_SRC_DIR ${ZEPHYR_BASE}/samples/bluetooth/${NET_APP})

	ExternalZephyrProject_Add(
		APPLICATION ${NET_APP}
		SOURCE_DIR  ${NET_APP_SRC_DIR}
		BOARD       ${SB_CONFIG_NET_CORE_BOARD}
	)

  if(SB_CONFIG_TESTER_AUDIO)
    # Apply the LE Audio controller configuration file for any board that enables TESTER_AUDIO
    set(${NET_APP}_EXTRA_CONF_FILE
      ${APP_DIR}/overlay-le-audio-bt_ll_sw_split.conf
      CACHE INTERNAL ""
    )
  endif()

	native_simulator_set_child_images(${DEFAULT_IMAGE} ${NET_APP})
endif()


if(SB_CONFIG_TESTER_AUDIO)
  # Apply the LE Audio configuration file for any non-netcore board if TESTER_AUDIO is set
  if(DEFINED EXTRA_CONF_FILE)
    # Append if other EXTRA_CONF_FILE is set
    set(EXTRA_CONF_FILE "${EXTRA_CONF_FILE};${APP_DIR}/overlay-le-audio.conf" CACHE INTERNAL "")
  else()
    set(EXTRA_CONF_FILE ${APP_DIR}/overlay-le-audio.conf CACHE INTERNAL "")
  endif()
endif()

native_simulator_set_final_executable(${DEFAULT_IMAGE})
