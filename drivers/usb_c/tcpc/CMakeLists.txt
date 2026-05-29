# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_FUSB307 fusb307.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_NUMAKER ucpd_numaker.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_PS8XXX ps8xxx.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_RT1715 rt1715.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_STM32 ucpd_stm32.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_TCPCI tcpci.c)
# zephyr-keep-sorted-stop
