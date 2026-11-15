# Copyright (c) 2025 Syswonder
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")

# pinctrl_soc.h is in this leaf SoC directory, not SOC_FULL_DIR.
set(rk3588_pinctrl_soc_dir "${CMAKE_CURRENT_SOURCE_DIR}")
zephyr_include_directories("${rk3588_pinctrl_soc_dir}")
