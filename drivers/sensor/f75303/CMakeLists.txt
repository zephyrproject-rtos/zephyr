# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(f75303.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_F75303 f75303_emul.c)
