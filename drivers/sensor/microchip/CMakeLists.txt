# Copyright (c) 2024 Analog Devices, Inc.
# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_MCP9600 mcp9600)
add_subdirectory_ifdef(CONFIG_MCP970X mcp970x)
add_subdirectory_ifdef(CONFIG_MTCH9010 mtch9010)
add_subdirectory_ifdef(CONFIG_TACH_XEC mchp_tach_xec)
add_subdirectory_ifdef(CONFIG_TCN75A tcn75a)
# zephyr-keep-sorted-stop
