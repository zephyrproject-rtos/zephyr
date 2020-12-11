#
# Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
# SPDX-License-Identifier: Apache-2.0
#

config OSDP_NUM_CONNECTED_PD
	int "Number of connected Peripheral Devices"
	default 1
	range 1 126
	help
	  In PD mode, number of connected PDs is is always 1 and cannot
	  be configured.

config OSDP_PD_ADDRESS_LIST
	string "List of connected Peripheral Device addresses"
	default "1"
	help
	  Comma Separated Values of PD addresses. The number of values in this
	  string should exactly match the number of connected PDs specified above

config OSDP_PD_COMMAND_QUEUE_SIZE
	int "OSDP Peripheral Device command queue size"
	default 16
	help
	  The number of commands that can be queued to a given PD. In CP mode,
	  the queue size is multiplied by number of connected PD so this can grow
	  very quickly.

config OSDP_CMD_RETRY_WAIT_SEC
	int "Retry wait time in seconds after a command error"
	default 30
	help
	  Time in seconds to wait after a command failure, and before retrying or
	  issuing further commands.

config OSDP_PD_POLL_RATE
	int "Number Peripheral Device POLL commands per second"
	default 20
	help
	  The Control Panel must query the Peripheral Device periodically to
	  maintain connection sequence and to get status and events. This option
	  defined the number of times such a POLL command is sent per second.

if OSDP_SC_ENABLED

config OSDP_MASTER_KEY
	string "Secure Channel Master Key"
	default "NONE"
	help
	  Hexadecimal string representation of the the 16 byte OSDP Secure Channel
	  master Key. This is a mandatory key when secure channel is enabled.

endif # OSDP_SC_ENABLED
