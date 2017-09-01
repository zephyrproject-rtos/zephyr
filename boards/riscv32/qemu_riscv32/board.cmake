set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -m ${CONFIG_RISCV_RAM_SIZE_MB}
  -nographic
  -machine sifive
  )

# TODO: Support debug
# set(DEBUG_SCRIPT qemu.sh)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
