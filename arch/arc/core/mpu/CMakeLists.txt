# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_if_kconfig(arc_core_mpu.c)
zephyr_library_sources_if_kconfig(arc_mpu.c)
