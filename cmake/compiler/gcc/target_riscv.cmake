# SPDX-License-Identifier: Apache-2.0

set(riscv_mabi "lp")
set(riscv_march "rv")

if(CONFIG_RISCV_CMODEL_MEDLOW)
  set(riscv_mcmodel "medlow")
elseif(CONFIG_RISCV_CMODEL_MEDANY)
  set(riscv_mcmodel "medany")
elseif(CONFIG_RISCV_CMODEL_LARGE)
  set(riscv_mcmodel "large")
endif()

if(CONFIG_64BIT)
  string(APPEND riscv_mabi "64")
  string(APPEND riscv_march "64")
else()
  string(CONCAT riscv_mabi  "i" ${riscv_mabi} "32")
  string(APPEND riscv_march "32")
endif()

if(CONFIG_RISCV_ISA_RV32E)
  string(APPEND riscv_mabi "e")
  string(APPEND riscv_march "e")
else()
  string(APPEND riscv_march "i")
endif()

if(CONFIG_RISCV_ISA_EXT_M)
  string(APPEND riscv_march "m")
endif()

if(CONFIG_RISCV_ISA_EXT_A)
  string(APPEND riscv_march "a")
endif()

if(CONFIG_FLOAT_HARD)
  if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    string(APPEND riscv_mabi "d")
  else()
    string(APPEND riscv_mabi "f")
  endif()
endif()

if(CONFIG_FPU)
  if(CONFIG_RISCV_ISA_EXT_F)
    string(APPEND riscv_march "f")
  endif()
  if(CONFIG_RISCV_ISA_EXT_D)
    string(APPEND riscv_march "d")
  endif()
endif()

if(CONFIG_RISCV_ISA_EXT_C)
  string(APPEND riscv_march "c")
endif()

if(CONFIG_RISCV_ISA_EXT_ZICNTR)
  string(APPEND riscv_march "_zicntr")
endif()

if(CONFIG_RISCV_ISA_EXT_ZICSR)
  string(APPEND riscv_march "_zicsr")
endif()

if(CONFIG_RISCV_ISA_EXT_ZIFENCEI)
  string(APPEND riscv_march "_zifencei")
endif()

# Check whether we already imply Zaamo/Zalrsc by selecting the A extension; if not - check them
# individually and enable them as needed
if(NOT CONFIG_RISCV_ISA_EXT_A)
  if(CONFIG_RISCV_ISA_EXT_ZAAMO)
    string(APPEND riscv_march "_zaamo")
  endif()

  if(CONFIG_RISCV_ISA_EXT_ZALRSC)
    string(APPEND riscv_march "_zalrsc")
  endif()
endif()

# Zca is implied by C
if(CONFIG_RISCV_ISA_EXT_ZCA AND
   NOT CONFIG_RISCV_ISA_EXT_C)
  string(APPEND riscv_march "_zca")
endif()

if(CONFIG_RISCV_ISA_EXT_ZCB)
  string(APPEND riscv_march "_zcb")
endif()

# Zcd is implied by C+D
if(CONFIG_RISCV_ISA_EXT_ZCD AND
   NOT (CONFIG_RISCV_ISA_EXT_C AND CONFIG_RISCV_ISA_EXT_D))
  string(APPEND riscv_march "_zcd")
endif()

# Zcf is implied by C+F
if(CONFIG_RISCV_ISA_EXT_ZCF AND
   NOT (CONFIG_RISCV_ISA_EXT_C AND CONFIG_RISCV_ISA_EXT_F))
  string(APPEND riscv_march "_zcf")
endif()

if(CONFIG_RISCV_ISA_EXT_ZCMP)
  string(APPEND riscv_march "_zcmp")
endif()

if(CONFIG_RISCV_ISA_EXT_ZCMT)
  string(APPEND riscv_march "_zcmt")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBA)
  string(APPEND riscv_march "_zba")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBB)
  string(APPEND riscv_march "_zbb")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBC)
  string(APPEND riscv_march "_zbc")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBKB)
  string(CONCAT riscv_march ${riscv_march} "_zbkb")
endif()

if(CONFIG_RISCV_ISA_EXT_ZBS)
  string(APPEND riscv_march "_zbs")
endif()

# Check whether we already imply Zmmul by selecting the M extension; if not - enable it
if(NOT CONFIG_RISCV_ISA_EXT_M AND
   CONFIG_RISCV_ISA_EXT_ZMMUL AND
   "${GCC_COMPILER_VERSION}" VERSION_GREATER_EQUAL 13.0.0)
  string(APPEND riscv_march "_zmmul")
endif()

list(APPEND RISCV_C_FLAGS
     -mabi=${riscv_mabi}
     -march=${riscv_march}
     -mcmodel=${riscv_mcmodel}
     )
list(APPEND TOOLCHAIN_C_FLAGS ${RISCV_C_FLAGS})
list(APPEND TOOLCHAIN_GROUPED_LD_FLAGS RISCV_C_FLAGS)

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
  -mcmodel=${riscv_mcmodel}
  -mno-relax
  -msmall-data-limit=0
  )
