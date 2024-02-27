# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

if(CONFIG_BOARD_QEMU_XTENSA)
  set(QEMU_CPU_TYPE_${ARCH} dc233c)

  set(QEMU_FLAGS_${ARCH}
    -machine sim -semihosting -nographic -cpu dc233c
  )
endif()

# TODO: Support debug
# board_set_debugger_ifnset(qemu)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
