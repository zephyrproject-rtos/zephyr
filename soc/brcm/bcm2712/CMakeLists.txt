# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PINCTRL pinctrl_soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
