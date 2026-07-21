# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

# Translate the SoC name and part number into the imx device and cpu
# name respectively.
string(TOUPPER ${CONFIG_SOC} IMX_DEVICE)

# Build imx drivers and utilities that can be used for multiple SoC's.
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/imx/drivers hal_nxp/imx/drivers)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/imx/devices hal_nxp/imx/devices)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/imx/devices/${IMX_DEVICE} hal_nxp/imx/devices/soc)
