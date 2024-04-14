# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(lps22hb.c)
zephyr_library_sources_ifdef(CONFIG_LPS22HB_TRIGGER    lps22hb_trigger.c)

zephyr_library_include_directories(../stmemsc)
