# Custom targets for compiler and linker flags.
add_custom_target(asm)
add_custom_target(compiler)
add_custom_target(compiler-cpp)
add_custom_target(linker)

# Loading of templates are strictly not needed as they does not set any
# properties.
# They purely provides an overview as well as a starting point for supporting
# a new toolchain.
include(${CMAKE_CURRENT_LIST_DIR}/compiler/compiler_flags_template.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/linker/linker_flags_template.cmake)

# Configure the toolchain flags based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${TOOLCHAIN_ROOT}/cmake/compiler/${COMPILER}/compiler_flags.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/linker/${LINKER}/linker_flags.cmake OPTIONAL)
