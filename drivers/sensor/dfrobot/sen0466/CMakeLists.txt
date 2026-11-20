# Copyright (c) 2026, Iomico
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(sen0466.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_SEN0466 sen0466_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_SEN0466 .)
