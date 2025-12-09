# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_RISCV_CUSTOM_CSR_VEXRISCV_CACHE cache_vexriscv.c)
