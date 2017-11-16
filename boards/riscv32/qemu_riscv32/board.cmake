set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -m ${CONFIG_RISCV_RAM_SIZE_MB}
  -nographic
  -machine sifive
  )

set(BOARD_DEBUG_RUNNER qemu)
