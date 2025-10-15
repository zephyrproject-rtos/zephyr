# Copyright (c) 2025 Andes Technology Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_PMA pma.c)
zephyr_sources_ifndef(CONFIG_INCLUDE_RESET_VECTOR reset.S)
zephyr_linker_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_EXECIT RODATA SORT_KEY 0x0 execit.ld)
zephyr_linker_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_PMA RAM_SECTIONS SORT_KEY 0x0 pma_align_start.ld)
zephyr_linker_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES_PMA RAM_SECTIONS SORT_KEY z_end pma_align_end.ld)
