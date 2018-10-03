# Kconfig - console driver configuration options

#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

menuconfig WEBSOCKET_CONSOLE
	bool "Enable websocket console service"
	select NETWORKING
	select NET_TCP
	select HTTP_PARSER
	select HTTP_SERVER
	select WEBSOCKET
	help
	This option enables console over a websocket link. Currently,
	it is basically just a redirection of the Zephyr console through
	websocket. It nicely works along with another console driver (like
	uart), twist being that it will take over the output only if a
	successful connection to its HTTP service is done.

if WEBSOCKET_CONSOLE

config WEBSOCKET_CONSOLE_LINE_BUF_SIZE
	int "WS console line buffer size"
	default 128
	help
	This option can be used to modify the size of the buffer storing
	console output line, prior to sending it through the network.
	Of course an output line can be longer than such size, it just
	means sending it will start as soon as it reaches this size.
	It really depends on what type of output is expected.
	If there is a lot of short lines, then lower this value. If there
	are longer lines, then raise this value.

config WEBSOCKET_CONSOLE_LINE_BUF_NUMBERS
	int "WS console line buffers"
	default 4
	help
	This option can be used to modify the amount of line buffers the
	driver can use. It really depends on how much output is meant to be
	sent, depending on the system load etc. You can play on both
	WEBSOCKET_CONSOLE_LINE_BUF_SIZE and this current option to get the
	best possible buffer settings you need.

config WEBSOCKET_CONSOLE_SEND_TIMEOUT
	int "WS console line send timeout"
	default 100
	help
	This option can be used to modify the duration of the timer that kick
	in when a line buffer is not empty but did not yet meet the line feed.

config WEBSOCKET_CONSOLE_SEND_THRESHOLD
	int "WS console line send threshold"
	default 5
	help
	This option can be used to modify the minimal amount of a line buffer
	that can be sent by the WS server when nothing has happened for
	a little while (see WEBSOCKET_CONSOLE_SEND_TIMEOUT) and when the line
	buffer did not meet the line feed yet.

config WEBSOCKET_CONSOLE_STACK_SIZE
	int "WS console inner thread stack size"
	default 1500
	help
	This option helps to fine-tune WS console inner thread stack size.

config WEBSOCKET_CONSOLE_PRIO
	int "WS console inner thread priority"
	default 7
	help
	This option helps to fine-tune WS console inner thread priority.

config SYS_LOG_WEBSOCKET_CONSOLE_LEVEL
	int "WS console log level"
	default 0
	depends on SYS_LOG
	help
	Sets log level for websocket console (for WS console dev only)

	Levels are:

	- 0 OFF, do not write

	- 1 ERROR, only write SYS_LOG_ERR

	- 2 WARNING, write SYS_LOG_WRN in addition to previous level

	- 3 INFO, write SYS_LOG_INF in addition to previous levels

	- 4 DEBUG, write SYS_LOG_DBG in addition to previous levels

config WEBSOCKET_CONSOLE_DEBUG_DEEP
	bool "Forward output to original console handler"
	depends on UART_CONSOLE
	help
	For WS console developers only, this will forward each output to
	original console handler. So if by chance WS console seems silent,
	at least things will be printed to original handler, usually
	UART console.

config WEBSOCKET_CONSOLE_INIT_PRIORITY
	int "WS console init priority"
	default 99
	help
	WS console driver initialization priority. Note that WS works
	on application level. Usually, you won't have to tweak this.

endif
