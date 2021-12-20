# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m33)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine mps2-an521
  -nographic
  -m 16
  -vga none
  )
board_set_debugger_ifnset(qemu)

if (CONFIG_BUILD_WITH_TFM)
  # Override the binary used by qemu, to use the combined
  # TF-M (Secure) & Zephyr (Non Secure) image (when running
  # in-tree tests).
  set(QEMU_KERNEL_OPTION "-device;loader,file=${CMAKE_BINARY_DIR}/tfm_merged.hex")

elseif (CONFIG_SOC_MPS2_AN521_CPU1)
  set(CPU0_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/zephyr/boards/arm/mps2_an521/empty_cpu0-prefix/src/empty_cpu0-build/zephyr)
  set(QEMU_KERNEL_OPTION "-device;loader,file=${CPU0_BINARY_DIR}/zephyr.elf")
  set(QEMU_EXTRA_FLAGS "-device;loader,file=${CMAKE_CURRENT_BINARY_DIR}/zephyr/zephyr.elf")
endif()
