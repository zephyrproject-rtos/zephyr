# Copyright (c) 2024 Bosch Sensortec GmbH

# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(bmm350.c bmm350_i2c.c)
zephyr_library_sources_ifdef(CONFIG_BMM350_TRIGGER bmm350_trigger.c)
