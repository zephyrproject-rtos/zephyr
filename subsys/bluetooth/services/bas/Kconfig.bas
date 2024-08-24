# Bluetooth GATT Battery service

# Copyright (c) 2018 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

config BT_BAS
	bool "GATT Battery service"

config BT_BAS_BLS
	bool "Battery Level Status"
	help
	Enable this option to include Battery Level Status Characteristic.

if BT_BAS_BLS

config BT_BAS_BLS_IDENTIFIER_PRESENT
	bool "Battery Level Identifier Present"
	help
	Enable this option if the Battery Level Identifier is present.

config BT_BAS_BLS_BATTERY_LEVEL_PRESENT
	bool "Battery Level Present"
	help
	Enable this option if the Battery Level is present.

config BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT
	bool "Additional Battery Status Present"
	help
	Enable this option if Additional Battery Status information is present.
endif
