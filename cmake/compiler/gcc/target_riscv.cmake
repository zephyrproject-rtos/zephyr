# SPDX-License-Identifier: Apache-2.0

set(riscv_mabi "lp")
set(riscv_march "rv")

if(CONFIG_64BIT)
    string(CONCAT riscv_mabi  ${riscv_mabi} "64")
    string(CONCAT riscv_march ${riscv_march} "64")
    list(APPEND TOOLCHAIN_C_FLAGS -mcmodel=medany)
    list(APPEND TOOLCHAIN_LD_FLAGS -mcmodel=medany)
else()
    string(CONCAT riscv_mabi  "i" ${riscv_mabi} "32")
    string(CONCAT riscv_march ${riscv_march} "32")
endif()

if (CONFIG_RISCV_ISA_RV32E)
    string(CONCAT riscv_mabi ${riscv_mabi} "e")
    string(CONCAT riscv_march ${riscv_march} "e")
else()
    string(CONCAT riscv_march ${riscv_march} "i")
endif()

if (CONFIG_RISCV_ISA_EXT_M)
    string(CONCAT riscv_march ${riscv_march} "m")
endif()
if (CONFIG_RISCV_ISA_EXT_A)
    string(CONCAT riscv_march ${riscv_march} "a")
endif()

if(CONFIG_FPU)
    if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
        if(CONFIG_FLOAT_HARD)
            string(CONCAT riscv_mabi ${riscv_mabi} "d")
        endif()
        string(CONCAT riscv_march ${riscv_march} "fd")
    else()
        if(CONFIG_FLOAT_HARD)
            string(CONCAT riscv_mabi ${riscv_mabi} "f")
        endif()
        string(CONCAT riscv_march ${riscv_march} "f")
    endif()
endif()

if(CONFIG_RISCV_ISA_EXT_C)
    string(CONCAT riscv_march ${riscv_march} "c")
endif()

if(CONFIG_RISCV_ISA_EXT_ZICSR)
    string(CONCAT riscv_march ${riscv_march} "_zicsr")
endif()

if(CONFIG_RISCV_ISA_EXT_ZIFENCEI)
    string(CONCAT riscv_march ${riscv_march} "_zifencei")
endif()

# Check whether we already imply Zaamo/Zlrsc by selecting the A extension; if not - check them
# individually and enable them as needed
if(NOT CONFIG_RISCV_ISA_EXT_A)
  if(CONFIG_RISCV_ISA_EXT_ZAAMO)
    string(CONCAT riscv_march ${riscv_march} "_zaamo")
  endif()

  if(CONFIG_RISCV_ISA_EXT_ZLRSC)
    string(CONCAT riscv_march ${riscv_march} "_zlrsc")
  endif()
endif()

if(CONFIG_RISCV_ISA_EXT_ZBA)
    string(CONCAT riscv_march ${riscv_march} "_zba")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBB)
    string(CONCAT riscv_march ${riscv_march} "_zbb")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBC)
    string(CONCAT riscv_march ${riscv_march} "_zbc")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBS)
    string(CONCAT riscv_march ${riscv_march} "_zbs")
endif()

list(APPEND TOOLCHAIN_C_FLAGS -mabi=${riscv_mabi} -march=${riscv_march})
list(APPEND TOOLCHAIN_LD_FLAGS NO_SPLIT -mabi=${riscv_mabi} -march=${riscv_march})

# Flags not supported by llext linker
# (regexps are supported and match whole word)
set(LLEXT_REMOVE_FLAGS
  -fno-pic
  -fno-pie
  -ffunction-sections
  -fdata-sections
  -g.*
  -Os
)

# Flags to be added to llext code compilation
# mno-relax is needed to stop gcc from generating R_RISCV_ALIGN relocations,
# which are currently not supported
# -msmall-data-limit=0 disables the "small data" sections such as .sbss and .sdata
# only one NOBITS sections is supported at a time, so having .sbss can cause
# llext's not to be loadable
set(LLEXT_APPEND_FLAGS
  -mabi=${riscv_mabi}
  -march=${riscv_march}
  -mno-relax
  -msmall-data-limit=0
)
