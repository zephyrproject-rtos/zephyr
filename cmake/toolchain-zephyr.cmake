# NB: Depends on host-tools.cmake having been executed already

# Ensures toolchain-gcc.cmake is run
set(COMPILER gcc)

# TODO fetch from environment

# These defaults work for some targets like RISC-V
set(CROSS_COMPILE_TARGET ${ARCH}-${TOOLCHAIN_VENDOR}-elf)
set(SYSROOT_TARGET       ${ARCH}-${TOOLCHAIN_VENDOR}-elf)

if("${ARCH}" STREQUAL "arm")
  set(CROSS_COMPILE_TARGET   arm-${TOOLCHAIN_VENDOR}-eabi)
  set(SYSROOT_TARGET       armv5-${TOOLCHAIN_VENDOR}-eabi)

elseif(CONFIG_TOOLCHAIN_VARIANT STREQUAL "iamcu")
  set(CROSS_COMPILE_TARGET  i586-${TOOLCHAIN_VENDOR}-elfiamcu)
  set(SYSROOT_TARGET       iamcu-${TOOLCHAIN_VENDOR}-elfiamcu)

elseif("${ARCH}" STREQUAL "x86")
  set(CROSS_COMPILE_TARGET i586-${TOOLCHAIN_VENDOR}-elf)
  set(SYSROOT_TARGET       i586-${TOOLCHAIN_VENDOR}-elf)

elseif("${ARCH}" STREQUAL "xtensa")
  set(SYSROOT_DIR ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/${SYSROOT_TARGET}/usr)
  set(TOOLCHAIN_INCLUDES
    ${SYSROOT_DIR}/include/arch/include
    ${SYSROOT_DIR}/include
    )

  LIST(APPEND TOOLCHAIN_LIBS hal)
  LIST(APPEND LIB_INCLUDE_DIR -L${SYSROOT_DIR}/lib)

elseif("${ARCH}" STREQUAL "arc")
  # https://github.com/zephyrproject-rtos/zephyr/issues/3797
  # -Os is broken on arc
  set(OPTIMIZE_FOR_SIZE_FLAG "-O2")
endif()

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/usr/bin/${CROSS_COMPILE_TARGET}/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/${SYSROOT_TARGET}/usr)

list(APPEND TOOLCHAIN_C_FLAGS --sysroot ${SYSROOT_DIR})
