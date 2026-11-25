# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)

# QEMU emulation uses the Xilinx arm-generic-fdt machine and a hardware device
# tree ("hw-dtb") built by CMakeLists.txt from the optional Xilinx/qemu-devicetrees
# west module (group "optional"). It is enabled only when that module is present.
# The Xilinx QEMU is shipped by the Zephyr SDK as qemu-system-xilinx-aarch64.
if(EXISTS "${WEST_TOPDIR}/modules/hal/qemu_devicetrees/Makefile")
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_BINARY_SUFFIX xilinx-aarch64)
  set(QEMU_CPU_TYPE cortex-r5f)

  set(QEMU_BOARD_FLAGS
    -machine arm-generic-fdt
    -hw-dtb ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
    # RST_LPD_TOP (0xff5e023c): deassert the R5 cluster reset
    -device loader,addr=0xff5e023c,data=0x80088fde,data-len=4
    # RPU_0 config (0xff9a0000): set RVBAR / release R5 core 0
    -device loader,addr=0xff9a0000,data=0x80000218,data-len=4
    -nographic
    -m 4g
  )

  set(QEMU_KERNEL_OPTION
    -device loader,cpu-num=4,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
  )
endif()
