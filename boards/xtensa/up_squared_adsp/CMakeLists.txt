# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(EXISTS ${BOARD_DIR}/bootloader/CMakeLists.txt)
  if(USING_OUT_OF_TREE_BOARD)
    set(build_dir boards/${ARCH}/${BOARD}/bootloader)
  else()
    unset(build_dir)
  endif()

  add_subdirectory(${BOARD_DIR}/bootloader ${build_dir})
endif()

set(sof_module ${PROJECT_SOURCE_DIR}/../modules/audio/sof)

add_custom_target(
  process_elf ALL
  DEPENDS base_module
  DEPENDS ${ZEPHYR_FINAL_EXECUTABLE}
  COMMAND ${CMAKE_OBJCOPY} --dump-section .data=mod-apl.bin ${CMAKE_BINARY_DIR}/zephyr/boards/xtensa/${BOARD}/bootloader/libbase_module.a
  COMMAND ${CMAKE_OBJCOPY} --add-section .module=mod-apl.bin --set-section-flags .module=load,readonly ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf.mod
  )

add_custom_target(
  process_bootloader ALL
  DEPENDS bootloader boot_module
  COMMAND ${CMAKE_OBJCOPY} --dump-section .data=mod-boot.bin ${CMAKE_BINARY_DIR}/zephyr/boards/xtensa/${BOARD}/bootloader/libboot_module.a
  COMMAND ${CMAKE_OBJCOPY} --add-section .module=mod-boot.bin --set-section-flags .module=load,readonly ${CMAKE_BINARY_DIR}/zephyr/boards/xtensa/${BOARD}/bootloader/bootloader.elf ${CMAKE_BINARY_DIR}/zephyr/bootloader.elf.mod
  )
