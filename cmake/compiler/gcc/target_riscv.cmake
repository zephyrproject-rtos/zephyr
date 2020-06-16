# SPDX-License-Identifier: Apache-2.0

set(riscv_mabi "lp")
set(riscv_march "rv")

if(CONFIG_64BIT)
    string(CONCAT riscv_mabi  ${riscv_mabi} "64")
    string(CONCAT riscv_march ${riscv_march} "64ima")
    list(APPEND TOOLCHAIN_C_FLAGS -mcmodel=medany)
    list(APPEND TOOLCHAIN_LD_FLAGS -mcmodel=medany)
else()
    string(CONCAT riscv_mabi  "i" ${riscv_mabi} "32")
    string(CONCAT riscv_march ${riscv_march} "32ima")
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

if(CONFIG_COMPRESSED_ISA)
    string(CONCAT riscv_march ${riscv_march} "c")
endif()

list(APPEND TOOLCHAIN_C_FLAGS -mabi=${riscv_mabi} -march=${riscv_march})

# The double quotes below ensure that the `-mabi` and `-march` flags are
# evaluated together by `target_ld_options`. This is necessary because
# specifying only either one of the flags will cause compilation to fail and
# that will effectively omit these flags.
list(APPEND TOOLCHAIN_LD_FLAGS "-mabi=${riscv_mabi} -march=${riscv_march}")
