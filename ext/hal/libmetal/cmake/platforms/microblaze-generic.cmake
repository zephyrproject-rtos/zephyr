    set (CMAKE_SYSTEM_PROCESSOR "microblaze"              CACHE STRING "")
    set (MACHINE "microblaze_generic" CACHE STRING "")
    set (CROSS_PREFIX           "mb-" CACHE STRING "")
    # These flags are for a demo. If microblaze is changed, the flags need to be changed too.
    set (CMAKE_C_FLAGS          "-mlittle-endian  -mxl-barrel-shift -mxl-pattern-compare  \
       -mcpu=v10.0 -mno-xl-soft-mul"   CACHE STRING "")
    include (cross-generic-gcc)
