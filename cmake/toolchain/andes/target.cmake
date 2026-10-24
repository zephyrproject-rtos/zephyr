# SPDX-License-Identifier: Apache-2.0

# Andes toolchain naming rule:
#   nds[32|64]le-[elf|linux]-[mculib|newlib|glibc]-[v5e|v5|v5f|v5d]
# The kernel configuration must match the toolchain for ABI
cmake_path(GET ANDES_TOOLCHAIN_PATH FILENAME TOOLCHAIN_BASENAME)

string(REPLACE "-" ";" TOOLCHAIN_PARTS "${TOOLCHAIN_BASENAME}")
list(GET TOOLCHAIN_PARTS 0 TOOLCHAIN_ARCH) # nds32le / nds64le
list(GET TOOLCHAIN_PARTS 3 TOOLCHAIN_ISA)  # v5e / v5 / v5f / v5d

# Check toolchain architecture
# TOOLCHAIN_ARCH == nds32le <=> CONFIG_64BIT=n
# TOOLCHAIN_ARCH == nds64le <=> CONFIG_64BIT=y
if((TOOLCHAIN_ARCH STREQUAL "nds32le" AND CONFIG_64BIT) OR
   (TOOLCHAIN_ARCH STREQUAL "nds64le" AND NOT CONFIG_64BIT))
  message(FATAL_ERROR "${TOOLCHAIN_BASENAME} (${TOOLCHAIN_ARCH}) mismatches CONFIG_64BIT")
endif()

# Check toolchain ISA
# TOOLCHAIN_ISA == v5e <=> CONFIG_RISCV_ISA_RV32E=y
# TOOLCHAIN_ISA != v5e <=> CONFIG_RISCV_ISA_RV32E=n
if((TOOLCHAIN_ISA STREQUAL "v5e" AND NOT CONFIG_RISCV_ISA_RV32E) OR
   (NOT TOOLCHAIN_ISA STREQUAL "v5e" AND CONFIG_RISCV_ISA_RV32E))
  message(FATAL_ERROR "${TOOLCHAIN_BASENAME} (${TOOLCHAIN_ISA}) mismatches CONFIG_RISCV_ISA_RV32E")
endif()

# Check float ABI based on toolchain ISA
# The libraries (libgcc.a, libc.a, ...) of an Andes toolchain are built only
# for the float ABI given by its ISA suffix, and Zephyr derives -mabi from
# CONFIG_FLOAT_HARD and CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION:
# TOOLCHAIN_ISA == v5/v5e <=> soft-float ABI   (CONFIG_FLOAT_HARD=n)
# TOOLCHAIN_ISA == v5f    <=> single-float ABI (CONFIG_FLOAT_HARD=y,
#                                               CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION=n)
# TOOLCHAIN_ISA == v5d    <=> double-float ABI (CONFIG_FLOAT_HARD=y,
#                                               CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION=y)
if(TOOLCHAIN_ISA STREQUAL "v5" OR TOOLCHAIN_ISA STREQUAL "v5e")
  if(CONFIG_FLOAT_HARD)
    message(FATAL_ERROR "${TOOLCHAIN_BASENAME} only supports the soft-float ABI, "
                        "set CONFIG_FLOAT_HARD=n or use a v5f/v5d toolchain")
  endif()
elseif(TOOLCHAIN_ISA STREQUAL "v5f")
  if(NOT CONFIG_FLOAT_HARD OR CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    message(FATAL_ERROR "${TOOLCHAIN_BASENAME} only supports the single-float ABI, "
                        "which requires CONFIG_FPU=y and CONFIG_FLOAT_HARD=y on a CPU "
                        "without double-precision FPU")
  endif()
elseif(TOOLCHAIN_ISA STREQUAL "v5d")
  if(NOT CONFIG_FLOAT_HARD OR NOT CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    message(FATAL_ERROR "${TOOLCHAIN_BASENAME} only supports the double-float ABI, "
                        "which requires CONFIG_FPU=y and CONFIG_FLOAT_HARD=y on a CPU "
                        "with double-precision FPU")
  endif()
endif()
