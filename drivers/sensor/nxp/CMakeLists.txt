# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_FXAS21002 fxas21002)
add_subdirectory_ifdef(CONFIG_FXLS8974 fxls8974)
add_subdirectory_ifdef(CONFIG_FXOS8700 fxos8700)
add_subdirectory_ifdef(CONFIG_MCUX_LPCMP mcux_lpcmp)
add_subdirectory_ifdef(CONFIG_NXP_TEMPMON nxp_tempmon)
add_subdirectory_ifdef(CONFIG_P3T1755 p3t1755)
add_subdirectory_ifdef(CONFIG_QDEC_MCUX qdec_mcux)
add_subdirectory_ifdef(CONFIG_QDEC_NXP_S32 qdec_nxp_s32)
add_subdirectory_ifdef(CONFIG_SENSOR_MCUX_ACMP mcux_acmp)
add_subdirectory_ifdef(CONFIG_TEMP_KINETIS nxp_kinetis_temp)
# zephyr-keep-sorted-stop
