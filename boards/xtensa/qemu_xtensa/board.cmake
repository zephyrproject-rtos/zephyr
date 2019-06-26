# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} sample_controller)
set_property(TARGET   ${ZEPHYR_TARGET} PROPERTY   QEMU_FLAGS_${ARCH}
	-machine sim -semihosting -nographic -cpu sample_controller
  )

# TODO: Support debug
# board_set_debugger_ifnset(qemu)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
