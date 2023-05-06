# Makefile - Atmel SAM0 MCU family
#
# Copyright (c) 2019 ML!PA Consulting GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc_port.c)

zephyr_sources_ifdef(CONFIG_BOOTLOADER_BOSSA bossa.c)

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMC20 soc_samc2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMC21 soc_samc2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMD20 soc_samd2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMD21 soc_samd2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMR21 soc_samd2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAML21 soc_saml2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMR34 soc_saml2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMR35 soc_saml2x.c)
zephyr_sources_ifdef(CONFIG_SOC_ATMEL_SAMR3X_RADIO_OFF_SETUP soc_samr3x_radio_off.c)

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMD51 soc_samd5x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAME51 soc_samd5x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAME53 soc_samd5x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAME54 soc_samd5x.c)

zephyr_include_directories(.)
