#
# Copyright (c) 2018 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LIS2MDL}         lis2mdl.c
            IFDEF:${CONFIG_LIS2MDL_TRIGGER} lis2mdl_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
