set(TOOLCHAIN_HOME /opt/xtensa/XtDevTools/install/tools/$ENV{TOOLCHAIN_VER}/XtensaTools)

set(COMPILER gcc)

set(CROSS_COMPILE_TARGET xt)
set(SYSROOT_TARGET       xtensa-elf)

set(CROSS_COMPILE  ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR    ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})
set(XCC_BUILD      $ENV{XTENSA_BUILD_PATHS}/$ENV{TOOLCHAIN_VER}/${CONFIG_SOC})

# xt-xcc does not support -Og, so make it -O0
set(OPTIMIZE_FOR_DEBUG_FLAG "-O0")

set(CC xcc)
set(C++ xc++)

list(APPEND TOOLCHAIN_C_FLAGS -fms-extensions)
