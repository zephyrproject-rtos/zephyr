# Configures CMake for using GNU tools, this script is re-used by several
# GCC-based toolchains
find_program(CMAKE_OBJCOPY    ${CROSS_COMPILE}objcopy PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP    ${CROSS_COMPILE}objdump PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AR         ${CROSS_COMPILE}ar      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB     ${CROSS_COMPILE}ranlib  PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_READELF    ${CROSS_COMPILE}readelf PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_GDB        ${CROSS_COMPILE}gdb     PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_NM         ${CROSS_COMPILE}nm      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

include(${ZEPHYR_BASE}/cmake/bintools/api_binutils.cmake)
