#
# Copyright (c) 2020 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(iis2mdc.c)
zephyr_library_sources(iis2mdc_i2c.c)
zephyr_library_sources(iis2mdc_spi.c)
zephyr_library_sources_ifdef(CONFIG_IIS2MDC_TRIGGER iis2mdc_trigger.c)

zephyr_library_include_directories(../stmemsc)
