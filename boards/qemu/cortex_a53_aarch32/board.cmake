# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-a53)

# secure=on     : QEMU starts the primary CPU at EL3 so the shim can run there
#                 and ERET down to AArch32 EL1 (the shim then sets SCR_EL3.NS=1,
#                 so Zephyr itself runs Non-secure)
# EL2 is intentionally left out (no virtualization=on): the boot path never uses
# it, and the shim must not touch EL2 registers that would then be unimplemented.
set(QEMU_MACH virt,secure=on,gic-version=3)

if(CONFIG_ENTROPY_VIRTIO)
  set(QEMU_VIRTIO_ENTROPY_FLAGS -device virtio-rng-device,bus=virtio-mmio-bus.0)
endif()

# QEMU starts the primary CPU at EL3 in AArch64 at the -bios entry point. The
# board CMakeLists.txt builds shim.bin, which downgrades to AArch32 EL1 and
# enters Zephyr. Zephyr is loaded at 0x60000000, above the device tree that
# QEMU's "virt" machine places at the base of RAM (0x40000000).
set(QEMU_KERNEL_OPTION
  -bios ${CMAKE_BINARY_DIR}/shim/shim.bin
  -device loader,file=${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin,addr=0x60000000
  )

set(QEMU_FLAGS_${ARCH}
  -global virtio-mmio.force-legacy=false
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  ${QEMU_VIRTIO_ENTROPY_FLAGS}
  # RAM spans from 0x40000000; 1 GiB is enough to back Zephyr at 0x60000000.
  -m 1024
  -nographic
  -machine ${QEMU_MACH}
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
