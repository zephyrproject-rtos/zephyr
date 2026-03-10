# SPDX-License-Identifier: Apache-2.0

zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_no_misleading_indentation>>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,warning_no_misleading_indentation>>)

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
