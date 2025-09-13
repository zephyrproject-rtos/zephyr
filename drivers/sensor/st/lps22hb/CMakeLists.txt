# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(lps22hb.c)

zephyr_library_sources_ifdef(CONFIG_EMUL_LPS22HB emul_lps22hb.c)