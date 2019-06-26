# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

if(NOT CONFIG_REBOOT)
  set(REBOOT_FLAG -no-reboot)
endif()

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} qemu32,+nx,+pae)
set_property(TARGET   ${ZEPHYR_TARGET} PROPERTY   QEMU_FLAGS_${ARCH}
  -m 12
  -cpu qemu32,+nx,+pae
  -device isa-debug-exit,iobase=0xf4,iosize=0x04
  ${REBOOT_FLAG}
  -nographic
  -no-acpi
  )

# TODO: Support debug
# board_set_debugger_ifnset(qemu)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
