# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TACH_ENE_KB1200	tach_ene_kb1200.c)
