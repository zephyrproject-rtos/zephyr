set (CMAKE_SYSTEM_PROCESSOR "arm"            CACHE STRING "")
set (MACHINE                "zynq7"          CACHE STRING "")
set (CROSS_PREFIX           "arm-none-eabi-" CACHE STRING "")

set (CMAKE_C_FLAGS          "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard" CACHE STRING "")

include (cross-freertos-gcc)

# vim: expandtab:ts=2:sw=2:smartindent
