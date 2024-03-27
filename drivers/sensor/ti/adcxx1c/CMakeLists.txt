# Copyright (c) 2024 Bert Outtier
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(adcxx1c.c)
zephyr_library_sources_ifdef(CONFIG_ADCXX1C_TRIGGER adcxx1c_trigger.c)
