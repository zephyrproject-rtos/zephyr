# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE leon3)

# This board used to pass "-icount auto" here, where it was silently overridden
# by the CONFIG_QEMU_ICOUNT_SHIFT derived value. Honouring it instead fails the
# kernel timer and scheduler tests, so the derived value is the one this board
# wants; the flag was dead and is dropped rather than moved to
# QEMU_ICOUNT_OVERRIDE.
set(QEMU_FLAGS_${ARCH}
  -machine leon3_generic
  -m 1G
  )
include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
