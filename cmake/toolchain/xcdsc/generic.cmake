zephyr_get(XCDSC_TOOLCHAIN_PATH)
# Set toolchain module name to xcdsc
set(COMPILER xcdsc)
set(LINKER xcdsc)
set(BINTOOLS xcdsc)
# Set base directory where binaries are available
if(XCDSC_TOOLCHAIN_PATH)
    set(TOOLCHAIN_HOME ${XCDSC_TOOLCHAIN_PATH}/bin/)
    set(XCDSC_BIN_PREFIX xc-dsc-)
    set(CROSS_COMPILE ${XCDSC_TOOLCHAIN_PATH}/bin/xc-dsc-)
endif()
if(NOT EXISTS ${XCDSC_TOOLCHAIN_PATH})
    message(FATAL_ERROR "Nothing found at XCDSC_TOOLCHAIN_PATH: '${XCDSC_TOOLCHAIN_PATH}'")
else() # Support for picolibc is indicated by the presence of 'picolibc.h' in the toolchain path.
    file(GLOB_RECURSE picolibc_header ${XCDSC_TOOLCHAIN_PATH}/include/picolibc/picolibc.h)
    if(picolibc_header)
        set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "True if toolchain supports picolibc")
        endif()
endif()
