# Copyright (c) 2021 Linaro
# Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

# The FVP variant must be used to enable Ethos-U55 NPU support, but QEMU also
# supports the AN547 without the NPU.
#
# Default emulation:
#     QEMU is used by default for corstone300/an547 and
#     FVP  is used by default for corstone300/fvp.
#


if(CONFIG_BOARD_MPS3_CORSTONE300_AN547 OR CONFIG_BOARD_MPS3_CORSTONE300_AN547_NS)
  set(SUPPORTED_EMU_PLATFORMS qemu)

  # QEMU settings
  set(QEMU_CPU_TYPE_${ARCH} cortex-m55)
  set(QEMU_FLAGS_${ARCH}
    -cpu ${QEMU_CPU_TYPE_${ARCH}}
    -machine mps3-an547
    -nographic
    -vga none
    )
elseif(CONFIG_BOARD_MPS3_CORSTONE300_FVP OR CONFIG_BOARD_MPS3_CORSTONE300_FVP_NS)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_Corstone_SSE-300_Ethos-U55)
elseif(CONFIG_BOARD_MPS3_CORSTONE300)
  string(REPLACE "mps3/corstone300;" "" board_targets "${board_targets}")
  string(REPLACE ";" "\n" board_targets "${board_targets}")
  message(FATAL_ERROR "Please use a target from the list below: \n${board_targets}\n")
endif()

board_set_debugger_ifnset(qemu)

if (CONFIG_BUILD_WITH_TFM)
  # Override the binary used by qemu, to use the combined
  # TF-M (Secure) & Zephyr (Non Secure) image (when running
  # in-tree tests).
  set(QEMU_KERNEL_OPTION "-device;loader,file=${CMAKE_BINARY_DIR}/zephyr/tfm_merged.hex")
endif()

# FVP Parameters
# -C indicate a config option in the form of:
#   instance.parameter=value
# Run the FVP with --list-params to list all options
set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
  -C mps3_board.uart0.out_file=-
  -C mps3_board.uart0.unbuffered_output=1
  -C mps3_board.uart1.out_file=-
  -C mps3_board.uart1.unbuffered_output=1
  -C mps3_board.uart2.out_file=-
  -C mps3_board.uart2.unbuffered_output=1
  -C mps3_board.visualisation.disable-visualisation=1
  -C mps3_board.telnetterminal0.start_telnet=0
  -C mps3_board.telnetterminal1.start_telnet=0
  -C mps3_board.telnetterminal2.start_telnet=0
  )
