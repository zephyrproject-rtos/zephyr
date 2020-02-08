# Bluetooth GATT Battery service

# Copyright (c) 2018 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

menuconfig BT_GATT_BAS
	bool "Enable GATT Battery service"
	select SENSOR

config BT_GATT_BAS_LOG_LEVEL
	int "Battery service log level"
	depends on LOG
	range 0 4
	default 0
	depends on BT_GATT_BAS
	help
	  Sets log level for the Battery service.
	  Levels are:
	  0 OFF, do not write
	  1 ERROR, only write LOG_ERR
	  2 WARNING, write LOG_WRN in addition to previous level
	  3 INFO, write LOG_INF in addition to previous levels
	  4 DEBUG, write LOG_DBG in addition to previous levels
