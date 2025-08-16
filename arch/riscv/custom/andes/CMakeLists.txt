# Copyright (c) 2025 Andes Technology Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_PMA pma.c)
zephyr_sources_ifndef(CONFIG_INCLUDE_RESET_VECTOR reset.S)
zephyr_linker_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_EXECIT RODATA SORT_KEY 0x0 execit.ld)
zephyr_linker_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_PMA RAM_SECTIONS SORT_KEY 0x0 pma_align_start.ld)
zephyr_linker_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_PMA RAM_SECTIONS SORT_KEY 0x0 pma_align_end.ld)

# Note: AndeStar V5 DSP needs custom Andes V5 toolchain
if(CONFIG_RISCV_CUSTOM_CSR_ANDES_HWDSP)
  zephyr_cc_option(-mext-dsp)
endif()

# Note: AndeStar V5 EXEC.IT needs custom Andes V5 toolchain
if(CONFIG_RISCV_CUSTOM_CSR_ANDES_EXECIT)
  zephyr_cc_option(-mexecit)
  zephyr_ld_options(-Wl,--mexecit)
endif()
