
set(TOOLCHAIN_VARIANT_COMPILER gcc CACHE STRING "Variant compiler being used")
set(COMPILER host-gcc)
set(LINKER ld)
set(BINTOOLS host-gnu)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: host (gcc/ld)")
