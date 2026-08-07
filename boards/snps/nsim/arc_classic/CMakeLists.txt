# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_ARC_MPU_ENABLE arc_mpu_regions.c)
zephyr_sources_ifdef(CONFIG_ISA_ARCV3 haps_arcv3_init.c)
