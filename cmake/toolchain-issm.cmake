#
# The ISSM standalone toolchain provides both the x86 IAMCU and elf32 ARC
# toolchains. Currently supported version is '2016-05-12':
# https://software.intel.com/en-us/articles/issm-toolchain-only-download
#

set_ifndef(ISSM_INSTALLATION_PATH $ENV{ISSM_INSTALLATION_PATH})
set(       ISSM_INSTALLATION_PATH    ${ISSM_INSTALLATION_PATH} CACHE PATH "")
assert(    ISSM_INSTALLATION_PATH     "ISSM_INSTALLATION_PATH is not set")

set(COMPILER gcc)

set(TOOLCHAIN_VENDOR intel)

# IA_VERSION and ARC_VERSION can be used to adjust the toolchain paths
# inside the ISSM installation to use an specific version, e.g.:
set_ifndef(IA_VERSION gcc-ia/5.2.1)
set_ifndef(ARC_VERSION gcc-arc/4.8.5)

if("${ARCH}" STREQUAL "x86")
  set(CROSS_COMPILE_TARGET i586-${TOOLCHAIN_VENDOR}-elfiamcu)
  set(specific_version ${IA_VERSION})
elseif("${ARCH}" STREQUAL "arc")
  set(CROSS_COMPILE_TARGET arc-elf32)
  set(specific_version ${ARC_VERSION})
else()
  message(FATAL_ERROR
    "issm was selected as the toolchain and ${ARCH} as the ARCH, \
but issm only supports x86 and arc"
    )
endif()

set(SYSROOT_TARGET ${CROSS_COMPILE_TARGET})

set(TOOLCHAIN_HOME ${ISSM_INSTALLATION_PATH}/tools/compiler/${specific_version})

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})


# TODO: What was _version used for?
