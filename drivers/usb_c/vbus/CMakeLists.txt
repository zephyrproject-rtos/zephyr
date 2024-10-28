# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USBC_VBUS_ADC usbc_vbus_adc.c)
zephyr_library_sources_ifdef(CONFIG_USBC_VBUS_NUMAKER usbc_vbus_numaker.c)
zephyr_library_sources_ifdef(CONFIG_USBC_VBUS_TCPCI usbc_vbus_tcpci.c)
