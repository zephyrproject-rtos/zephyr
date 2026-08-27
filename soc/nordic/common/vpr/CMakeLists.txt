# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(soc_context.S soc_init.c)
zephyr_library_sources_ifdef(CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE soc_idle.c)

zephyr_compile_options(-msmall-data-limit=0)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT AND NOT CONFIG_OMIT_FRAME_POINTER)
  # Enabling FP is possible, but the VPR will crash if FP are enabled for leaf functions.
  zephyr_cc_option(-momit-leaf-frame-pointer)
endif()
