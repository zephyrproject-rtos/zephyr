# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_BT smp_bt.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_SHELL smp_shell.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_UART smp_uart.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_UDP smp_udp.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_DUMMY smp_dummy.c)

if (CONFIG_MCUMGR_SMP_SHELL OR CONFIG_MCUMGR_SMP_UART)
    zephyr_library_sources(serial_util.c)
endif()
