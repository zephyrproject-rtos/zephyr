include(${TOOLCHAIN_ROOT}/cmake/bintools/api_binutils_generate_isr.cmake)
include(${TOOLCHAIN_ROOT}/cmake/bintools/api_binutils_usermode.cmake)
include(${TOOLCHAIN_ROOT}/cmake/bintools/api_binutils_print_size.cmake)
include(${TOOLCHAIN_ROOT}/cmake/bintools/api_binutils_post_link.cmake)

# Do not combine multiple macros into 1; more detailed control is needed
