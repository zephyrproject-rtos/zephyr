# Bluetooth Audio - Call Control Profile (CCP) configuration options
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if BT_AUDIO

config BT_CCP_SERVER
	bool "Call Control Profile Server Support"
	depends on BT_EXT_ADV
	depends on BT_TBS
	depends on BT_BONDABLE
	help
	  This option enables support for the Call Control Profile Server which uses the Telephone
	  Bearer Service (TBS) to hold and control calls on a device.

if BT_CCP_SERVER

config BT_CCP_SERVER_BEARER_COUNT
	int "Telephone bearer count"
	default 1
	range 1 $(UINT8_MAX)
	help
	  The number of supported telephone bearers on the CCP Server

module = BT_CCP_SERVER
module-str = "Call Control Profile Server"
source "subsys/logging/Kconfig.template.log_config"

endif # BT_CCP_SERVER

endif # BT_AUDIO
