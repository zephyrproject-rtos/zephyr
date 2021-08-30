# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} nios2)

set(QEMU_FLAGS_${ARCH}
  -machine altera_10m50_zephyr
  -nographic
  )

board_set_debugger_ifnset(qemu)
