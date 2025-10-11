# Bluetooth Audio - Call Control Profile (CCP) configuration options
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if BT_AUDIO

config BT_CCP_CALL_CONTROL_CLIENT
	bool "Call Control Profile Client Support"
	depends on BT_EXT_ADV
	depends on BT_TBS_CLIENT
	depends on BT_BONDABLE
	help
	  This option enables support for the Call Control Profile Client which uses the Telephone
	  Bearer Service (TBS) client to control calls on a remote device.

if BT_CCP_CALL_CONTROL_CLIENT

config BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT
	int "Telephone bearer count"
	default 1
	range 1 $(UINT8_MAX) if BT_TBS_CLIENT_TBS
	range 1 1
	help
	  The number of supported telephone bearers on the CCP Call Control Client

module = BT_CCP_CALL_CONTROL_CLIENT
module-str = "Call Control Profile Client"
source "subsys/logging/Kconfig.template.log_config"

endif # BT_CCP_CALL_CONTROL_CLIENT

config BT_CCP_CALL_CONTROL_SERVER
	bool "Call Control Profile Call Control Server Support"
	depends on BT_EXT_ADV
	depends on BT_TBS
	depends on BT_BONDABLE
	help
	  This option enables support for the Call Control Profile Call Control Server which uses
	  the Telephone Bearer Service (TBS) to hold and control calls on a device.

if BT_CCP_CALL_CONTROL_SERVER

config BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT
	int "Telephone bearer count"
	default 1
	range 1 $(UINT8_MAX)
	help
	  The number of supported telephone bearers on the CCP Call Control Server

config BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH
	int "The maximum length of the bearer provider name excluding null terminator"
	default BT_TBS_MAX_PROVIDER_NAME_LENGTH
	range 1 BT_TBS_MAX_PROVIDER_NAME_LENGTH
	help
	  Sets the maximum length of the bearer provider name.

module = BT_CCP_CALL_CONTROL_SERVER
module-str = "Call Control Profile Call Control Server"
source "subsys/logging/Kconfig.template.log_config"

endif # BT_CCP_CALL_CONTROL_SERVER

endif # BT_AUDIO
