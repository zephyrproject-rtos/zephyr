# Copyright (c) 2025 Orgatex GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bq35100.c)
zephyr_library_sources_ifdef(CONFIG_BQ35100_CALIBRATION bq35100_calibration.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_BQ35100 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_BQ35100 emul_bq35100.c)
