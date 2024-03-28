# Copyright(c) 2023 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(adltc2990.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_ADLTC2990 adltc2990_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_ADLTC2990 .)
