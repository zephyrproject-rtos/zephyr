set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} nios2)

set(QEMU_FLAGS_${ARCH}
  -machine altera_10m50_zephyr
  -nographic
  )

set(BOARD_DEBUG_RUNNER qemu)
