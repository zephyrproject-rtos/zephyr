# SPDX-License-Identifier: Apache-2.0

add_custom_target(
  process_elf ALL
  DEPENDS sof_base_module
  DEPENDS ${ZEPHYR_FINAL_EXECUTABLE}
  COMMAND ${CMAKE_OBJCOPY} --dump-section .data=mod-apl.bin ${CMAKE_BINARY_DIR}/modules/sof/libsof_base_module.a
  COMMAND ${CMAKE_OBJCOPY} --add-section .module=mod-apl.bin --set-section-flags .module=load,readonly ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf.sof
  )
