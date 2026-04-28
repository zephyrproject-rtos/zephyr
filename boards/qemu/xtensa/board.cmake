# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

if(CONFIG_BOARD_QEMU_XTENSA)
  set(QEMU_CPU_TYPE_${ARCH} ${CONFIG_SOC})

  set(QEMU_FLAGS_${ARCH}
    -machine sim -semihosting -nographic -cpu ${CONFIG_SOC}
  )
endif()

# TODO: Support debug
# board_set_debugger_ifnset(qemu)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
