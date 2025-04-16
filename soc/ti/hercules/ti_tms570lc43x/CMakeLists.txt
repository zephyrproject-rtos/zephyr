# SPDX-License-Identifier: Apache-2.0

# TMS470 requires the ARM "legacy Word Invariant Addressing big-endian mode"
zephyr_compile_options(
  -mbe32
  ${TOOLCHAIN_C_FLAGS}
)

zephyr_ld_options(
  -mbe32
  ${TOOLCHAIN_LD_FLAGS}
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/arm/include
  )

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_r/scripts/linker.ld CACHE INTERNAL "")
