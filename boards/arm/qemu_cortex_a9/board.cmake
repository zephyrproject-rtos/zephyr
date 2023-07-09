#
# Copyright (c) 2021, Weidmueller Interface GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0
#

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH xilinx-aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-a9)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine arm-generic-fdt-7series
  -dtb ${ZEPHYR_BASE}/boards/${ARCH}/${BOARD}/fdt-zynq7000s.dtb
  )

set(QEMU_KERNEL_OPTION
  "-device;loader,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>,cpu-num=0"
  )

board_runner_args(qemu "--commander=qemu-system-xilinx-aarch64" "--cpu=cortex-a9" "--machine=arm-generic-fdt-7series"
                  "--dtb=${CMAKE_CURRENT_LIST_DIR}/fdt-zynq7000s.dtb"
                  "--qemu_option=${CMAKE_CURRENT_LIST_DIR}/qemu_cortex_a9_option.yml")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
