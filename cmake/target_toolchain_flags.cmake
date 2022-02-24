# Uniquely identify the toolchain wrt. its capabilities.
#
# What we are looking for, is a signature definition that is defined
# like this:
#  * The MD5 sum of the compiler itself. A MD5 checksum is taken of the content
#    after symlinks are resolved. This ensure that if the content changes, then
#    the MD5 will also change (as example toolchain upgrade in same folder)
#  * The CMAKE_C_COMPILER itself. This may be a symlink, but it ensures that
#    multiple symlinks pointing to same executable will generate different
#    signatures, as example: clang, gcc, arm-zephyr-eabi-gcc, links pointing to
#    ccache will generate unique signatures
#  * CMAKE_C_COMPILER_ID is taking the CMake compiler id for extra signature.
#  * CMAKE_C_COMPILER_VERSION will ensure that even when using the previous
#    methods, where an upgraded compiler could have same signature due to ccache
#    usage and symbolic links, then the upgraded compiler will have new version
#    and thus generate a new signature.
#
#  Toolchains with the same signature will always support the same set of flags.
#
file(MD5 ${CMAKE_C_COMPILER} CMAKE_C_COMPILER_MD5_SUM)
set(TOOLCHAIN_SIGNATURE ${CMAKE_C_COMPILER_MD5_SUM})

# Extend the CMAKE_C_COMPILER_MD5_SUM with the compiler signature.
string(MD5 COMPILER_SIGNATURE ${CMAKE_C_COMPILER}_${CMAKE_C_COMPILER_ID}_${CMAKE_C_COMPILER_VERSION})
set(TOOLCHAIN_SIGNATURE ${TOOLCHAIN_SIGNATURE}_${COMPILER_SIGNATURE})

# Load the compile features file which will provide compile features lists for
# various C / CXX language dialects that can then be exported based on current
# Zephyr Kconfig settings or the CSTD global property.
include(${CMAKE_CURRENT_LIST_DIR}/compiler/compiler_features.cmake)

# Loading of templates are strictly not needed as they do not set any
# properties.
# They purely provide an overview as well as a starting point for supporting
# a new toolchain.
include(${CMAKE_CURRENT_LIST_DIR}/compiler/compiler_flags_template.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/linker/linker_flags_template.cmake)

# Configure the toolchain flags based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${TOOLCHAIN_ROOT}/cmake/compiler/${COMPILER}/compiler_flags.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/linker/${LINKER}/linker_flags.cmake OPTIONAL)
