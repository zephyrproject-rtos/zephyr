#
#  Copyright (c) 2018 Analog Devices Inc.
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ADXL362}         adxl372.c
            IFDEF:${CONFIG_ADXL372_TRIGGER} adxl372_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
