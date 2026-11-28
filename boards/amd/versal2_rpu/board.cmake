#
# Copyright (c) 2025-2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)

# QEMU emulation uses the Xilinx arm-generic-fdt machine and a hardware device
# tree ("hw-dtb") built by CMakeLists.txt from the optional Xilinx/qemu-devicetrees
# west module (group "optional"). It is enabled only when that module is present.
# The Xilinx QEMU is shipped by the Zephyr SDK as qemu-system-xilinx-aarch64.
if(EXISTS "${WEST_TOPDIR}/modules/hal/qemu_devicetrees/Makefile")
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_BINARY_SUFFIX xilinx-aarch64)
  set(QEMU_CPU_TYPE cortex-r52)

  set(QEMU_BOARD_FLAGS
    -machine arm-generic-fdt
    -hw-dtb ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
    -device loader,addr=0xEB5E0310,data=0x2,data-len=4
    -nographic
    -m 2g
  )

  set(QEMU_KERNEL_OPTION
    -device loader,cpu-num=8,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
  )
endif()
