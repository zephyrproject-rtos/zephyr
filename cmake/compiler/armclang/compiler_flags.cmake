# First step is to inherit all properties from gcc, as clang is compatible with most flags.
include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)

# Required ASM flags when using armclang, this should be handled by CMake, but
# fails because of: https://gitlab.kitware.com/cmake/cmake/-/issues/19963
set_property(TARGET asm APPEND PROPERTY required "--target=${triple}")

# Only the ARM Compiler C library is currently supported.
set_compiler_property(PROPERTY nostdinc)

# Remove after testing that -Wshadow works
set_compiler_property(PROPERTY warning_shadow_variables)
