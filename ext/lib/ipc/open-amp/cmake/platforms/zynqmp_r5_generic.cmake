set (CMAKE_SYSTEM_PROCESSOR "arm" CACHE STRING "")
set (MACHINE                "zynqmp_r5" CACHE STRING "")
set (CROSS_PREFIX           "armr5-none-eabi-" CACHE STRING "")

# Xilinx SDK version earlier than 2017.2 use mfloat-abi=soft by default to generat libxil
set (CMAKE_C_FLAGS          "-mfloat-abi=hard -mfpu=vfpv3-d16 -mcpu=cortex-r5" CACHE STRING "")

include (cross_generic_gcc)

# vim: expandtab:ts=2:sw=2:smartindent
