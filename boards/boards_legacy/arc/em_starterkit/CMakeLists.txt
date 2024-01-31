# SPDX-License-Identifier: Apache-2.0

zephyr_sources(pmodmux.c)
zephyr_sources_ifdef(CONFIG_ARC_MPU_ENABLE arc_mpu_regions.c)
