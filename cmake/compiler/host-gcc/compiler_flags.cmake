# SPDX-License-Identifier: Apache-2.0
# SPDX-FileComment: IEC-61508-T2

# Load toolchain_cc-family compiler flags
# Significant overlap with freestanding gcc compiler so reuse it
include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)
