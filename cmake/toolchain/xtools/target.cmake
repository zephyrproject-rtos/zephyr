# SPDX-License-Identifier: Apache-2.0

set(CROSS_COMPILE_TARGET_arm         arm-zephyr-eabi)
set(CROSS_COMPILE_TARGET_nios2     nios2-zephyr-elf)
set(CROSS_COMPILE_TARGET_riscv   riscv64-zephyr-elf)
set(CROSS_COMPILE_TARGET_mips     mipsel-zephyr-elf)
set(CROSS_COMPILE_TARGET_xtensa   xtensa-zephyr-elf)
set(CROSS_COMPILE_TARGET_arc         arc-zephyr-elf)

if(CONFIG_X86_64)
  set(CROSS_COMPILE_TARGET_x86 x86_64-zephyr-elf)
else()
  set(CROSS_COMPILE_TARGET_x86 i586-zephyr-elf)
endif()

set(CROSS_COMPILE_TARGET ${CROSS_COMPILE_TARGET_${ARCH}})
set(SYSROOT_TARGET       ${CROSS_COMPILE_TARGET})

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)

if("${ARCH}" STREQUAL "xtensa")
  set(SYSROOT_DIR ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})
  set(TOOLCHAIN_INCLUDES
    ${SYSROOT_DIR}/include/arch/include
    ${SYSROOT_DIR}/include
    )

  LIST(APPEND TOOLCHAIN_LIBS hal)
  LIST(APPEND LIB_INCLUDE_DIR -L${SYSROOT_DIR}/lib)
endif()
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET}/${SYSROOT_TARGET})

if("${ARCH}" STREQUAL "x86")
  if(CONFIG_X86_64)
    if(CONFIG_LIB_CPLUSPLUS)
      # toolchain was built with a few missing bits in
      # the multilib configuration so we need to specify
      # where to find the 64-bit libstdc++.
      LIST(APPEND LIB_INCLUDE_DIR -L${SYSROOT_DIR}/lib64)
    endif()

    list(APPEND TOOLCHAIN_C_FLAGS -m64)
    list(APPEND TOOLCHAIN_LD_FLAGS -m64)
  else()
    list(APPEND TOOLCHAIN_C_FLAGS -m32)
    list(APPEND TOOLCHAIN_LD_FLAGS -m32)
  endif()
endif()
