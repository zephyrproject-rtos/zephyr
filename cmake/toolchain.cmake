set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})

set(BUILD_SHARED_LIBS OFF)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

if(NOT ZEPHYR_GCC_VARIANT)
  if(DEFINED ENV{ZEPHYR_GCC_VARIANT})
    set(ZEPHYR_GCC_VARIANT $ENV{ZEPHYR_GCC_VARIANT})
  elseif(CROSS_COMPILE OR CONFIG_CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE}))
    set(ZEPHYR_GCC_VARIANT cross-compile)
  endif()
endif()
set(ZEPHYR_GCC_VARIANT ${ZEPHYR_GCC_VARIANT} CACHE STRING "Zephyr GCC variant")
assert(ZEPHYR_GCC_VARIANT "")

if(CONFIG_ARCH_POSIX OR (ZEPHYR_GCC_VARIANT STREQUAL "host"))
  set(COMPILER host-gcc)
endif()

# Configure the toolchain based on what SDK/toolchain is in use.
if(NOT (COMPILER STREQUAL "host-gcc"))
  include(${ZEPHYR_BASE}/cmake/toolchain-${ZEPHYR_GCC_VARIANT}.cmake)
endif()

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${ZEPHYR_BASE}/cmake/toolchain-${COMPILER}.cmake OPTIONAL)
