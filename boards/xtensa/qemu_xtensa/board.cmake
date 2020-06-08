# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} sample_controller)

set(QEMU_FLAGS_${ARCH}
	-machine sim -semihosting -nographic -cpu sample_controller
  )

if(CONFIG_QEMU_ICOUNT)
  list(APPEND QEMU_EXTRA_FLAGS -icount shift=6,align=off,sleep=off -rtc clock=vm)
endif()
# TODO: Support debug
# board_set_debugger_ifnset(qemu)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
