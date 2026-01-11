#  Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
zephyr_library()

zephyr_library_sources(adxl313.c adxl313_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ADXL313_TRIGGER adxl313_trigger.c adxl313_decoder.c)
