# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix riscv32)
set(QEMU_CPU_TYPE_${ARCH} riscv32)

if(CONFIG_BOARD_QEMU_RISCV32 OR CONFIG_BOARD_QEMU_RISCV32_SMP)
  set(QEMU_FLAGS_${ARCH}
    -nographic
    -machine virt
    -bios none
    -m 256
    )
else()
  set(QEMU_FLAGS_${ARCH}
    -nographic
    -machine sifive_e
    )
endif()


board_set_debugger_ifnset(qemu)
