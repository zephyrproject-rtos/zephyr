# locate the required XC-DSC binary tools.
find_program(CMAKE_OBJCOPY NAMES ${XCDSC_BIN_PREFIX}objcopy PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP NAMES ${XCDSC_BIN_PREFIX}objdump PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_READELF NAMES ${XCDSC_BIN_PREFIX}readelf PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_STRIP NAMES ${XCDSC_BIN_PREFIX}strip PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
# Archive Rules Setup
SET(CMAKE_C_ARCHIVE_CREATE " <CMAKE_AR> -q <TARGET> <LINK_FLAGS> <OBJECTS>  -mdfp=\"${DFP_ROOT}/xc16\"")
SET(CMAKE_C_ARCHIVE_FINISH " <CMAKE_AR> -q <TARGET> -mdfp=\"${DFP_ROOT}/xc16\"")
# Include bin tool properties
include(${ZEPHYR_BASE}/cmake/bintools/xcdsc/target_bintools.cmake)
