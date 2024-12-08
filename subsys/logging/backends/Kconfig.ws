# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

config LOG_BACKEND_WS
	bool "Websocket backend"
	depends on WEBSOCKET_CONSOLE
	select LOG_OUTPUT
	default y
	help
	  Send console messages to websocket console.

if LOG_BACKEND_WS

config LOG_BACKEND_WS_MAX_BUF_SIZE
	int "Max message size"
	range 64 1500
	default 512
	help
	  Maximum size of the output string that is sent via websocket.

config LOG_BACKEND_WS_TX_RETRY_CNT
	int "Number of TX retries"
	default 2
	help
	  Number of TX retries before dropping the full line of data.

config LOG_BACKEND_WS_TX_RETRY_DELAY_MS
	int "Delay between TX retries in milliseconds"
	default 50
	help
	  Sleep period between TX retry attempts.

config LOG_BACKEND_WS_AUTOSTART
	bool "Automatically start websocket backend"
	default y if NET_CONFIG_NEED_IPV4 || NET_CONFIG_NEED_IPV6
	help
	  When enabled automatically start the websocket backend on
	  application start.

backend = WS
backend-str = websocket
source "subsys/logging/Kconfig.template.log_format_config"

endif # LOG_BACKEND_WS
