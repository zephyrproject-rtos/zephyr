# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_ESPRESSIF_ESP32)
  set(LLVM_XTENSA_TARGET "xtensa-esp-elf")
else()
  set(LLVM_XTENSA_TARGET "xtensa-zephyr-elf")
endif()

list(APPEND TOOLCHAIN_C_FLAGS "--target=${LLVM_XTENSA_TARGET}" "-gz=none" "-Wa,--compress-debug-sections=none")
list(APPEND TOOLCHAIN_CXX_FLAGS "--target=${LLVM_XTENSA_TARGET}" "-gz=none" "-Wa,--compress-debug-sections=none")
list(APPEND TOOLCHAIN_ASM_FLAGS "--target=${LLVM_XTENSA_TARGET}" "-gz=none" "-Wa,--compress-debug-sections=none")
list(APPEND TOOLCHAIN_LD_FLAGS "--target=${LLVM_XTENSA_TARGET}" "-gz=none" "-Wa,--compress-debug-sections=none")


include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_xtensa.cmake)
