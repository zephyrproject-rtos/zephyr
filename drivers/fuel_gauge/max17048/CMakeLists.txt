# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(max17048.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_MAX17048 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_MAX17048 ./emul_max17048.c)
