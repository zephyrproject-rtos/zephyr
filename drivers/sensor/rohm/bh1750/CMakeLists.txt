# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bh1750.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_BH1750 emul_bh1750.c)
