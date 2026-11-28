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
  message(WARNING "${ANDES_TOOLCHAIN_PATH} (rv${TOOLCHAIN_XLEN}) mismatches CONFIG_64BIT. "
                  "The build is expected to fail due to an incompatible ABI.")
endif()

# Check toolchain ISA
# TOOLCHAIN_RVE <=> CONFIG_RISCV_ISA_RV32E=y
if((TOOLCHAIN_RVE AND NOT CONFIG_RISCV_ISA_RV32E) OR
   (NOT TOOLCHAIN_RVE AND CONFIG_RISCV_ISA_RV32E))
  message(WARNING "${ANDES_TOOLCHAIN_PATH} mismatches CONFIG_RISCV_ISA_RV32E. "
                  "The build is expected to fail due to an incompatible ABI.")
endif()
