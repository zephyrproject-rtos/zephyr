set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m3)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine lm3s6965evb
  -nographic
  -vga none
  )

set(BOARD_DEBUG_RUNNER qemu)
