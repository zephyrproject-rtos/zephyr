# SPDX-License-Identifier: Apache-2.0

# CCAC/MetaWare (MWDT) RISC-V target configuration

# Derive core (e.g. -av5rmx) and ISA feature flags (-Z*)
# Strictly from SoC Kconfig booleans (no board heuristics).
if(CONFIG_SOC_SERIES_RMX)
  set(ARCMWDT_RISCV_CORE -av5rmx)
elseif(CONFIG_SOC_SERIES_RHX)
  set(ARCMWDT_RISCV_CORE -av5rhx)
endif()

if(NOT ARCMWDT_RISCV_CORE)
  message(FATAL_ERROR
    "MWDT: Unable to determine ARC-V series "
    "from SoC Kconfig (expect CONFIG_SOC_SERIES_RMX "
    "or CONFIG_SOC_SERIES_RHX, etc.).")
endif()

# Translate Zephyr RISC-V ISA Kconfig options to MWDT (ccac) flags
set(ARCMWDT_RISCV_ISA_FLAGS)

# Base ISA extensions
if(CONFIG_RISCV_ISA_EXT_M)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zm)
endif()

if(CONFIG_RISCV_ISA_EXT_A)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Za)
endif()

# System and fence extensions
if(CONFIG_RISCV_ISA_EXT_ZICSR)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zicsr)
endif()

if(CONFIG_RISCV_ISA_EXT_ZIFENCEI)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zifencei)
endif()

# Base counters/timers
if(CONFIG_RISCV_ISA_EXT_ZICNTR)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zicntr)
endif()

# Floating point
if(CONFIG_RISCV_ISA_EXT_F)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zf)
endif()

if(CONFIG_RISCV_ISA_EXT_D)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zd)
endif()

# Bitmanip extensions
if(CONFIG_RISCV_ISA_EXT_ZBA)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zba)
endif()

if(CONFIG_RISCV_ISA_EXT_ZBB)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zbb)
endif()

if(CONFIG_RISCV_ISA_EXT_ZBC)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zbc)
endif()

if(CONFIG_RISCV_ISA_EXT_ZBS)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zbs)
endif()

# Compressed ISA and sub-extensions
# If full C is selected, use -Zc and add compressed FPU sub-exts as selected.
if(CONFIG_RISCV_ISA_EXT_C)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zc)
  if(CONFIG_RISCV_ISA_EXT_ZCF)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zcf)
  endif()
  if(CONFIG_RISCV_ISA_EXT_ZCD)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zcd)
  endif()
else()
  if(CONFIG_RISCV_ISA_EXT_ZCA)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zca)
  endif()
  if(CONFIG_RISCV_ISA_EXT_ZCB)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zcb)
  endif()
  if(CONFIG_RISCV_ISA_EXT_ZCD)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zcd)
  endif()
  if(CONFIG_RISCV_ISA_EXT_ZCF)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zcf)
  endif()
  if(CONFIG_RISCV_ISA_EXT_ZCMP)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zcmp)
  endif()
  if(CONFIG_RISCV_ISA_EXT_ZCMT)
    list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zcmt)
  endif()
endif()

# Multiply-only (when M is not selected)
if(NOT CONFIG_RISCV_ISA_EXT_M AND CONFIG_RISCV_ISA_EXT_ZMMUL)
  list(APPEND ARCMWDT_RISCV_ISA_FLAGS -Zmmul)
endif()

# Apply derived flags
if(ARCMWDT_RISCV_CORE)
  list(APPEND TOOLCHAIN_C_FLAGS ${ARCMWDT_RISCV_CORE})
  list(APPEND TOOLCHAIN_LD_FLAGS ${ARCMWDT_RISCV_CORE})
endif()

if(ARCMWDT_RISCV_ISA_FLAGS)
  list(APPEND TOOLCHAIN_C_FLAGS ${ARCMWDT_RISCV_ISA_FLAGS})
  list(APPEND TOOLCHAIN_LD_FLAGS ${ARCMWDT_RISCV_ISA_FLAGS})
endif()
