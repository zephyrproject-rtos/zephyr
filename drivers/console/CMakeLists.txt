# SPDX-License-Identifier: Apache-2.0

zephyr_sources_if_kconfig(uart_console.c)
zephyr_sources_if_kconfig(ram_console.c)
zephyr_sources_if_kconfig(rtt_console.c)
zephyr_sources_if_kconfig(ipm_console_receiver.c)
zephyr_sources_if_kconfig(ipm_console_sender.c)
zephyr_sources_if_kconfig(uart_mcumgr.c)
zephyr_sources_if_kconfig(uart_pipe.c)
zephyr_sources_if_kconfig(telnet_console.c)
zephyr_sources_if_kconfig(xtensa_sim_console.c)
zephyr_sources_if_kconfig(native_posix_console.c)
