set (CMAKE_SYSTEM_PROCESSOR "aarch64"           CACHE STRING "")
set (MACHINE                "zynqmp_a53"        CACHE STRING "")
set (CROSS_PREFIX           "aarch64-none-elf-" CACHE STRING "")
set (CMAKE_C_FLAGS          ""                  CACHE STRING "")

include (cross-generic-gcc)

# vim: expandtab:ts=2:sw=2:smartindent
