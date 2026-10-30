# Copyright (c) 2026 Federico Borgo
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(ltc3337.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_LTC3337 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_LTC3337 emul_ltc3337.c)
