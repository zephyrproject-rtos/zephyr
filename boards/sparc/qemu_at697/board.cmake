#
# Copyright (c) 2018 ispace, inc.
#
# SPDX-License-Identifier: Apache-2.0
#

set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} LEON2)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine at697
  -nographic
  )

set(BOARD_DEBUG_RUNNER qemu)
