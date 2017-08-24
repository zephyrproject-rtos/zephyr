set(EMU_PLATFORM qemu)

if(NOT CONFIG_REBOOT)
  set(REBOOT_FLAG -no-reboot)
endif()

set(QEMU_CPU_TYPE_${ARCH} qemu32)
set(QEMU_FLAGS_${ARCH}
  -m 8
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -device isa-debug-exit,iobase=0xf4,iosize=0x04
  ${REBOOT_FLAG}
  -nographic
  -vga none
  -display none
  -net none
  -clock dynticks
  -no-acpi
  -balloon none
  -L ${QEMU_BIOS}
  -bios bios.bin
  -machine type=pc-0.14
  )

# TODO: Support debug
# set(DEBUG_SCRIPT qemu.sh)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
