# Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_TI_CC35XX_HSM ti_cc35xx_hsm.c)
