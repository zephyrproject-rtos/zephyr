# SPDX-License-Identifier: Apache-2.0
set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix riscv32)
set(QEMU_CPU_TYPE_${ARCH} riscv32)

# Check if this is an AIA SoC (check CONFIG_SOC_QEMU_VIRT_RISCV32_AIA at configure time)
if(CONFIG_SOC_QEMU_VIRT_RISCV32_AIA)
  # AIA SoC - enable APLIC+IMSIC
  set(QEMU_FLAGS_${ARCH}
    -nographic
    -machine virt,aia=aplic-imsic
    -bios none
    -m 256M
  )
else()
  # Default PLIC SoC
  set(QEMU_FLAGS_${ARCH}
    -nographic
    -machine virt
    -bios none
    -m 256
  )
endif()

# Add SMP flags for SMP variant
if(CONFIG_SMP)
  list(APPEND QEMU_FLAGS_${ARCH} -smp ${CONFIG_MP_MAX_NUM_CPUS})
endif()

board_set_debugger_ifnset(qemu)
