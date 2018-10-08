set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  -smp ${CONFIG_MP_NUM_CPUS}
  )

set(BOARD_DEBUG_RUNNER qemu)
