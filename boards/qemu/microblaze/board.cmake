# Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
# Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
# SPDX-License-Identifier: Apache-2.0


set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH xilinx-microblazeel)

set(QEMU_CPU_TYPE_${ARCH} microblaze)

set(QEMU_FLAGS_${ARCH}
	-machine microblaze-fdt
	-nographic
	-hw-dtb ${ZEPHYR_BASE}/boards/qemu/${ARCH}/board-qemu-microblaze-demo.dtb
	# TODO: introduce a feature flags for debug flags
	# -D /scratch/esnap_asayin/mb.log   # if you enable anything from below, you probably want this too.
	# -d int      # show interrupts/exceptions in short format
	# -d in_asm   # show target assembly code for each compiled TB
	# -d exec     # show trace before each executed TB (lots of logs)
	# --trace "memory_region_ops_*"
	# --trace "exec_tb*"
)

set(QEMU_KERNEL_OPTION
	-kernel \$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
)

board_set_debugger_ifnset(qemu)

add_custom_target(debug_qemu
		COMMAND
		${CROSS_COMPILE}gdb
		-ex \"target extended-remote 127.0.0.1:${DEBUGSERVER_LISTEN_PORT}\"
		-ex \"set disassemble-next-line on\"
		${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
		USES_TERMINAL
)
