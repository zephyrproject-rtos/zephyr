# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LSM9DS0_MFD lsm9ds0_mfd.c)
