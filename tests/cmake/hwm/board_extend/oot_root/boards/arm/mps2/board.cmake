# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, Nordic Semiconductor ASA

if(CONFIG_BOARD_MPS2_AN521_CPUTEST)
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_CPU_TYPE_${ARCH} cortex-m33)
  set(QEMU_FLAGS_${ARCH}
    -cpu ${QEMU_CPU_TYPE_${ARCH}}
    -machine mps2-an521
    -nographic
    -m 16
    -vga none
    )
endif()
