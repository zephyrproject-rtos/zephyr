# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
  start.S
  soc_irq.S
)

zephyr_linker_sources(ROM_START SORT_KEY 0x0 common_linker/init.ld)
zephyr_linker_sources_ifdef(CONFIG_XIP RAM_SECTIONS SORT_KEY 0x0 common_linker/ram_start_nonzero.ld)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

# Note: AndeStar V5 DSP needs custom Andes V5 toolchain
if(CONFIG_RISCV_CUSTOM_CSR_ANDES_HWDSP)
  zephyr_cc_option(-mext-dsp)
endif()

# Note: AndeStar V5 EXEC.IT needs custom Andes V5 toolchain
if(CONFIG_RISCV_CUSTOM_CSR_ANDES_EXECIT)
  zephyr_cc_option(-mexecit)
  zephyr_ld_options(-Wl,--mexecit)
else()
  # Disable execit explicitly, as -Os enables it by default.
  zephyr_cc_option(-mno-execit)
  zephyr_ld_options(-Wl,--mno-execit)
endif()
