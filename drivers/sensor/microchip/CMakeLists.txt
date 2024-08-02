# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_MCP9600 mcp9600)
add_subdirectory_ifdef(CONFIG_MCP970X mcp970x)
add_subdirectory_ifdef(CONFIG_MCP9808 mcp9808)
add_subdirectory_ifdef(CONFIG_TACH_XEC mchp_tach_xec)
add_subdirectory_ifdef(CONFIG_TCN75A tcn75a)
# zephyr-keep-sorted-stop
