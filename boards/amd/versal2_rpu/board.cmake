#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)
set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH xilinx-aarch64)
set(QEMU_CPU_TYPE_${ARCH} cortex-r52)

set(QEMU_FLAGS_${ARCH}
	-machine arm-generic-fdt
	-hw-dtb ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
	-device loader,addr=0xEB5E0310,data=0x2,data-len=4
	-nographic
	-m 2g
)

set(QEMU_KERNEL_OPTION
	-device loader,cpu-num=8,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
)
