#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc.c
            IFDEF:${CONFIG_ARM_MPU} arm_mpu_regions.c
)

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})
