# Copyright (c) 2021-2025 ATL Electronics
# Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

zephyr_code_relocate_ifdef(CONFIG_UART_BFLB LIBRARY drivers__serial LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_RISCV_MACHINE_TIMER LIBRARY drivers__timer LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_PINCTRL_BFLB LIBRARY drivers__pinctrl LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_SYSCON_BFLB_EFUSE LIBRARY drivers__syscon LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_CLOCK_CONTROL_BOUFFALOLAB_BL60X
  LIBRARY drivers__clock_control LOCATION ITCM NOKEEP)
