# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix or1k)
set(QEMU_CPU_TYPE_${ARCH} or1k)

set(QEMU_FLAGS_${ARCH}
  -machine or1k-sim
  -nographic
)

board_set_debugger_ifnset(qemu)
