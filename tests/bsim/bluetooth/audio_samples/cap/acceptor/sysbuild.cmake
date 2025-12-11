# Copyright (c) 2023-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/samples/bluetooth/cap_acceptor/sysbuild.cmake)

native_simulator_set_primary_mcu_index(${DEFAULT_IMAGE} ${NET_APP})
