# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

set(SOC_FAMILY intel_adsp)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/bootloader/CMakeLists.txt)
  if(USING_OUT_OF_TREE_BOARD)
    set(build_dir boards/${ARCH}/${BOARD}/bootloader)
  else()
    unset(build_dir)
  endif()

  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bootloader ${build_dir})
endif()

set(ELF_FIX ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/common/fix_elf_addrs.py)

add_custom_target(
  process_elf ALL
  DEPENDS base_module
  DEPENDS ${ZEPHYR_FINAL_EXECUTABLE}
  COMMAND ${CMAKE_OBJCOPY} --dump-section .data=mod-apl.bin $<TARGET_FILE:base_module>
  COMMAND ${CMAKE_OBJCOPY} --add-section .module=mod-apl.bin --set-section-flags .module=load,readonly ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}.mod

  # Adjust final section addresses so they all appear in the cached region.
  COMMAND ${ELF_FIX} ${CMAKE_OBJCOPY} ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf.mod
  )

add_custom_target(
  process_bootloader ALL
  DEPENDS bootloader boot_module
  COMMAND ${CMAKE_OBJCOPY} --dump-section .data=mod-boot.bin $<TARGET_FILE:boot_module>
  COMMAND ${CMAKE_OBJCOPY} --add-section .module=mod-boot.bin --set-section-flags .module=load,readonly $<TARGET_FILE:bootloader> ${CMAKE_BINARY_DIR}/zephyr/bootloader.elf.mod
  )
