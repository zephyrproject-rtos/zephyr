set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})

set(BUILD_SHARED_LIBS OFF)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Configure the toolchain based on what SDK/toolchain is in use.
include($ENV{ZEPHYR_BASE}/cmake/toolchain-${ZEPHYR_GCC_VARIANT}.cmake)

# Configure the toolchain based on what toolchain technology is used
# (gcc clang etc.)
include($ENV{ZEPHYR_BASE}/cmake/toolchain-${COMPILER}.cmake OPTIONAL)
