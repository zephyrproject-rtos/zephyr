# SPDX-License-Identifier: Apache-2.0

# ARM1176-specific sources that replace the corresponding cortex_a_r files
# when CONFIG_ARMV6_ARM1176 is selected.

zephyr_library()

# reset.S and the assembly helpers include "vector_table.h", "boot.h", and
# "macro_priv.inc" which live in the sibling cortex_a_r/ directory.
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cortex_a_r)

zephyr_library_sources(
  reset.S
  cache.c
  cpu_idle.c
  fault.c
  thread.c
)

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifndef(CONFIG_USE_SWITCH swap_helper.S exc_exit.S)
