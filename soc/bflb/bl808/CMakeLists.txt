# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

if(CONFIG_BT_BFLB_BL808)
  zephyr_sources(bflb_rf.c)
  zephyr_sources(ble/btblecontroller_port_hw.c)
  zephyr_linker_sources(RWDATA bflb_rwdata.ld)
endif()
