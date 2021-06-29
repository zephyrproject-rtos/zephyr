# Copyright (c) 2021 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_CPU_HAS_ARM_MPU arm_mpu_regions.c)
zephyr_sources_ifdef(CONFIG_ENABLE_SOC_NMI  soc_nmi.c)
