# Bluetooth GATT Battery service

# Copyright (c) 2024 Croxel Inc.
# SPDX-License-Identifier: Apache-2.0

config BT_CTS
	bool "GATT Current Time service"

if BT_CTS

config BT_CTS_HELPER_API
	bool "Helper APIs to encode and decode CTS formatted time"

endif
