# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# Copyright (c) 2026 Open Device Partnership and Contributors
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bq4xz50.c)
zephyr_library_sources_ifdef(CONFIG_BQ40Z50 bq40z50.c)
zephyr_library_sources_ifdef(CONFIG_BQ41Z50 bq41z50.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_BQ4XZ50 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_BQ4XZ50 ./emul_bq4xz50.c)
