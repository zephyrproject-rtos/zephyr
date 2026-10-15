#
# Copyright (c) 2021, Weidmueller Interface GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0
#

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_BINARY_SUFFIX xilinx-aarch64)

set(QEMU_CPU_TYPE cortex-a9)

set(QEMU_BOARD_FLAGS
  -machine arm-generic-fdt-7series
  -dtb ${CMAKE_CURRENT_LIST_DIR}/fdt-zynq7000s.dtb
  )

if(CONFIG_XLNX_SDHC)
  find_program(QEMU_IMG_EXECUTABLE qemu-img)
  if(NOT QEMU_IMG_EXECUTABLE)
    message(FATAL_ERROR
      "qemu-img not found on PATH. It is required to generate the SD card "
      "image for CONFIG_XLNX_SDHC.\n"
      "  Linux:   sudo apt install qemu-utils  (or your distro's equivalent)\n"
      "  macOS:   brew install qemu\n"
      "  Windows: install from https://qemu.weilnetz.de/w64/, or use WSL/MSYS2"
    )
  endif()

  set(SD_IMAGE ${ZEPHYR_BINARY_DIR}/sdcard.img)

  if(NOT EXISTS ${SD_IMAGE})
    execute_process(
      COMMAND ${QEMU_IMG_EXECUTABLE} create -f raw ${SD_IMAGE} 64M
      RESULT_VARIABLE SD_IMAGE_CREATE_RESULT
    )
    if(NOT SD_IMAGE_CREATE_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to create SD card image at ${SD_IMAGE}")
    endif()
  endif()

  list(APPEND QEMU_BOARD_FLAGS
    -drive file=${SD_IMAGE},format=raw,if=sd,index=0
  )
endif()

set(QEMU_KERNEL_OPTION
  "-device;loader,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>,cpu-num=0"
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
