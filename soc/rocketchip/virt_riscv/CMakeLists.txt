# SPDX-License-Identifier: Apache-2.0

# Keep Zephyr’s RISC-V common linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

add_subdirectory(common)

# --- Chipyard/Rocketchip: language standard knobs for GCC 15 ---
# Force C to gnu17 and C++ to gnu++17 to avoid C23/C++20 surprises.
# These options are appended and should win over earlier defaults.
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:C>:-std=gnu17>
  $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
)
