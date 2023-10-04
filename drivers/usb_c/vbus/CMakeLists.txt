# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USBC_VBUS_ADC usbc_vbus_adc.c)
zephyr_library_sources_ifdef(CONFIG_USBC_VBUS_FUSB302B fusb_vbus.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_VBUS_FUSB302B emul_vbus_fusb302b.c)
