# Configures CMake for using GNU linker, this script is re-used by several
# GCC-based toolchains
find_program(CMAKE_LINKER     ${CROSS_COMPILE}ld      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

include(${ZEPHYR_BASE}/cmake/linker/api_ld.cmake)
