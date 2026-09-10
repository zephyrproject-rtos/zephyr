# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

zephyr_linker_sources(NOINIT ${CMAKE_CURRENT_SOURCE_DIR}/bflb_noinit.ld)
zephyr_linker_sources(RWDATA ${CMAKE_CURRENT_SOURCE_DIR}/bflb_rwdata.ld)
zephyr_linker_sources(ITCM_SECTION ${CMAKE_CURRENT_SOURCE_DIR}/bflb_itcm.ld)

if(CONFIG_BT_BFLB_BL70XL)
  # The BLE blob's ble_rwdata.ld defines __global_pointer$ which causes the
  # linker to relax address loads into GP-relative instructions. Since the
  # Zephyr CRT does not initialize GP (CONFIG_RISCV_GP is not enabled),
  # these relaxed instructions would crash. Disable linker relaxation so
  # only the blob's own pre-compiled GP-relative code uses GP.
  zephyr_ld_options(-Wl,--no-relax)
  zephyr_linker_sources(SECTIONS bflb_hbn.ld)

  zephyr_sources(
    ble/btblecontroller_port_hw.c
    bflb_platform_shim.c
    bflb_rf.c
    rf_callbacks.c
  )
endif()
