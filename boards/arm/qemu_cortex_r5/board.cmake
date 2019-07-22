# Copyright (c) 2019 Lexmark International, Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)
set(QEMU_ARCH aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-r5)
set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine xlnx-zcu102
  -global xlnx,zynqmp.boot-cpu="rpu-cpu[0]"
  -smp 6
  )

set(QEMU_KERNEL_OPTION
  "-device;loader,file=$<TARGET_FILE:zephyr_final>,cpu-num=4"
  )

board_set_debugger_ifnset(qemu)
