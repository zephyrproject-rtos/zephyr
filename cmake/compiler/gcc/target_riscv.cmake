# SPDX-License-Identifier: Apache-2.0

set(riscv_mabi "lp")
set(riscv_march "rv")

if(CONFIG_64BIT)
    string(CONCAT riscv_mabi  ${riscv_mabi} "64")
    string(CONCAT riscv_march ${riscv_march} "64ima")
    list(APPEND TOOLCHAIN_C_FLAGS -mcmodel=medany)
else()
    string(CONCAT riscv_mabi  "i" ${riscv_mabi} "32")
    string(CONCAT riscv_march ${riscv_march} "32ima")
endif()

if(CONFIG_FLOAT)
    string(CONCAT riscv_march ${riscv_march} "f")
endif()

if(CONFIG_COMPRESSED_ISA)
    string(CONCAT riscv_march ${riscv_march} "c")
endif()

list(APPEND TOOLCHAIN_C_FLAGS -mabi=${riscv_mabi} -march=${riscv_march})
list(APPEND TOOLCHAIN_LD_FLAGS -mabi=${riscv_mabi} -march=${riscv_march})
