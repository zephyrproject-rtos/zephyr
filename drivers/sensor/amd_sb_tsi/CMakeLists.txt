# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(sb_tsi.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_AMD_SB_TSI sb_tsi_emul.c)
