# SPDX-License-Identifier: Apache-2.0

# The kernel configuration must match the toolchain for ABI
string(REGEX MATCH "#define __riscv_xlen ([0-9]+)" _ "${ANDES_TOOLCHAIN_MACROS}")
set(TOOLCHAIN_XLEN ${CMAKE_MATCH_1})                  # 32 / 64

string(REGEX MATCH "#define __riscv_float_abi_([a-z]+)" _ "${ANDES_TOOLCHAIN_MACROS}")
set(TOOLCHAIN_FLOAT_ABI ${CMAKE_MATCH_1})             # soft / single / double

if(ANDES_TOOLCHAIN_MACROS MATCHES "#define __riscv_abi_rve ")
  set(TOOLCHAIN_RVE TRUE)
else()
  set(TOOLCHAIN_RVE FALSE)
endif()

# Check toolchain architecture
# TOOLCHAIN_XLEN == 32 <=> CONFIG_64BIT=n
# TOOLCHAIN_XLEN == 64 <=> CONFIG_64BIT=y
if((TOOLCHAIN_XLEN EQUAL 32 AND CONFIG_64BIT) OR
   (TOOLCHAIN_XLEN EQUAL 64 AND NOT CONFIG_64BIT))
  message(FATAL_ERROR "${ANDES_TOOLCHAIN_PATH} (rv${TOOLCHAIN_XLEN}) mismatches CONFIG_64BIT")
endif()

# Check toolchain ISA
# TOOLCHAIN_RVE <=> CONFIG_RISCV_ISA_RV32E=y
if((TOOLCHAIN_RVE AND NOT CONFIG_RISCV_ISA_RV32E) OR
   (NOT TOOLCHAIN_RVE AND CONFIG_RISCV_ISA_RV32E))
  message(FATAL_ERROR "${ANDES_TOOLCHAIN_PATH} mismatches CONFIG_RISCV_ISA_RV32E")
endif()

# Check float ABI
# The libraries (libgcc.a, libc.a, ...) of an Andes toolchain are built only
# for its float ABI, and Zephyr derives -mabi from CONFIG_FLOAT_HARD and
# CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION:
# TOOLCHAIN_FLOAT_ABI == soft   <=> CONFIG_FLOAT_HARD=n
# TOOLCHAIN_FLOAT_ABI == single <=> CONFIG_FLOAT_HARD=y,
#                                   CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION=n
# TOOLCHAIN_FLOAT_ABI == double <=> CONFIG_FLOAT_HARD=y,
#                                   CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION=y
if(TOOLCHAIN_FLOAT_ABI STREQUAL "soft")
  if(CONFIG_FLOAT_HARD)
    message(FATAL_ERROR "${ANDES_TOOLCHAIN_PATH} only supports the soft-float ABI, "
                        "set CONFIG_FLOAT_HARD=n or use a v5f/v5d toolchain")
  endif()
elseif(TOOLCHAIN_FLOAT_ABI STREQUAL "single")
  if(NOT CONFIG_FLOAT_HARD OR CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    message(FATAL_ERROR "${ANDES_TOOLCHAIN_PATH} only supports the single-float ABI, "
                        "which requires CONFIG_FPU=y and CONFIG_FLOAT_HARD=y on a CPU "
                        "without double-precision FPU")
  endif()
elseif(TOOLCHAIN_FLOAT_ABI STREQUAL "double")
  if(NOT CONFIG_FLOAT_HARD OR NOT CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    message(FATAL_ERROR "${ANDES_TOOLCHAIN_PATH} only supports the double-float ABI, "
                        "which requires CONFIG_FPU=y and CONFIG_FLOAT_HARD=y on a CPU "
                        "with double-precision FPU")
  endif()
endif()
