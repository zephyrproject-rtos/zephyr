## SPDX-FileCopyrightText: Copyright Bavariamatic GmbH
## SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_SOC_M2S010 m2s010)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")