# Copyright (c) 2021 Linaro
# SPDX-License-Identifier: Apache-2.0

# The AN547 FVP must be used to enable Ethos-U55 NPU support, but QEMU also
# supports the AN547 without the NPU.
#
# To use QEMU instead of the FVP as an emulation platform, set 'EMU_PLATFORM'
# to 'qemu' instead of 'armfvp', for example:
#
# $ west build -b mps3_an547 samples/hello°world -DEMU_PLATFORM=qemu -t run

if(NOT DEFINED EMU_PLATFORM)
  set(EMU_PLATFORM armfvp)
endif()

if (EMU_PLATFORM STREQUAL "qemu")
  # QEMU settings
  set(QEMU_CPU_TYPE_${ARCH} cortex-m55)
  set(QEMU_FLAGS_${ARCH}
    -cpu ${QEMU_CPU_TYPE_${ARCH}}
    -machine mps3-an547
    -nographic
    -vga none
    )
  board_set_debugger_ifnset(qemu)
else()
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
endif()
