set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m3)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine lm3s6965evb
  -nographic
  -vga none
  )

# TODO: Support debug
# set(DEBUG_SCRIPT qemu.sh)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
