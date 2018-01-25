set(EMU_PLATFORM qemu)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -vga none
  -display none
  -clock dynticks
  -semihosting
  )
