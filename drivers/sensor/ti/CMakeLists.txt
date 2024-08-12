# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_BQ274XX bq274xx)
add_subdirectory_ifdef(CONFIG_FDC2X1X fdc2x1x)
add_subdirectory_ifdef(CONFIG_INA219 ina219)
add_subdirectory_ifdef(CONFIG_INA226 ina226)
add_subdirectory_ifdef(CONFIG_INA23X ina23x)
add_subdirectory_ifdef(CONFIG_INA3221 ina3221)
add_subdirectory_ifdef(CONFIG_LM95234 lm95234)
add_subdirectory_ifdef(CONFIG_OPT3001 opt3001)
add_subdirectory_ifdef(CONFIG_TI_HDC ti_hdc)
add_subdirectory_ifdef(CONFIG_TI_HDC20XX ti_hdc20xx)
add_subdirectory_ifdef(CONFIG_TMAG5170 tmag5170)
add_subdirectory_ifdef(CONFIG_TMAG5273 tmag5273)
add_subdirectory_ifdef(CONFIG_TMP007 tmp007)
add_subdirectory_ifdef(CONFIG_TMP1075 tmp1075)
add_subdirectory_ifdef(CONFIG_TMP108 tmp108)
add_subdirectory_ifdef(CONFIG_TMP112 tmp112)
add_subdirectory_ifdef(CONFIG_TMP114 tmp114)
add_subdirectory_ifdef(CONFIG_TMP116 tmp116)
add_subdirectory_ifdef(CONFIG_TMP435 tmp435)
# zephyr-keep-sorted-stop
