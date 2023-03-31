# Copyright (c) 2021 Linaro
# SPDX-License-Identifier: Apache-2.0

# The AN547 FVP must be used to enable Ethos-U55 NPU support, but QEMU also
# supports the AN547 without the NPU.
#
# For emulation, QEMU is used by default. To use AN547 FVP as an emulation
# use the 'run_armfvp' target, for example:
#
# $ west build -b mps3_an547 samples/hello_world -t run_armfvp

set(SUPPORTED_EMU_PLATFORMS qemu armfvp)

# QEMU settings
set(QEMU_CPU_TYPE_${ARCH} cortex-m55)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine mps3-an547
  -nographic
  -vga none
  )
board_set_debugger_ifnset(qemu)

if (CONFIG_BUILD_WITH_TFM)
  # Override the binary used by qemu, to use the combined
  # TF-M (Secure) & Zephyr (Non Secure) image (when running
  # in-tree tests).
  set(QEMU_KERNEL_OPTION "-device;loader,file=${CMAKE_BINARY_DIR}/zephyr/merged.hex")
endif()

# FVP settings
set(ARMFVP_BIN_NAME FVP_Corstone_SSE-300_Ethos-U55)

# FVP Parameters
# -C indicate a config option in the form of:
#   instance.parameter=value
# Run the FVP with --list-params to list all options
set(ARMFVP_FLAGS
  -C mps3_board.uart0.out_file=-
  -C mps3_board.uart0.unbuffered_output=1
  -C mps3_board.uart1.out_file=-
  -C mps3_board.uart1.unbuffered_output=1
  -C mps3_board.uart2.out_file=-
  -C mps3_board.uart2.unbuffered_output=1
  -C mps3_board.visualisation.disable-visualisation=1
  )
