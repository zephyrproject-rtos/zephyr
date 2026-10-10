
set(TOOLCHAIN_VARIANT_COMPILER gnu CACHE STRING "Variant compiler being used")
set(COMPILER host-gcc)
if(CMAKE_HOST_APPLE)
  set(LINKER ld64)
  set(BINTOOLS host-darwin)
else()
  set(LINKER ld)
  set(BINTOOLS host-gnu)
endif()

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: host (gcc/ld)")
