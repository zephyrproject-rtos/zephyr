# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

include(${ZEPHYR_BASE}/boards/common/qemu_riscv.board.cmake)

qemu_riscv_cpu_from_dt(qemu_riscv_cpu)
qemu_riscv_binary_suffix(QEMU_BINARY_SUFFIX)

if(CONFIG_RISCV_S_MODE)
  # rv64i with sv39=on is broken in QEMU 10.x; use the full rv64 model instead.
  if(CONFIG_RISCV_MMU_SV48)
    set(qemu_riscv_cpu "rv64,s=on,u=on,pmp=on,priv_spec=v1.12.0,sv48=on")
  else()
    set(qemu_riscv_cpu "rv64,s=on,u=on,pmp=on,priv_spec=v1.12.0,sv39=on")
  endif()
endif()

set(QEMU_CPU_TYPE "${qemu_riscv_cpu}")

if(CONFIG_INPUT_VIRTIO)
  set(QEMU_VIRTIO_INPUT_FLAGS -device virtio-tablet-device,bus=virtio-mmio-bus.3)
endif()

set(QEMU_BOARD_FLAGS
  -machine virt
  -bios none
  -m 256
  -cpu ${qemu_riscv_cpu}
  ${QEMU_VIRTIO_INPUT_FLAGS}
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
