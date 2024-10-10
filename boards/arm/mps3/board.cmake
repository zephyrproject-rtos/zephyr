# Copyright (c) 2021 Linaro
# Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

# The AN552/AN547 FVP must be used to enable Ethos-U55 NPU support, but QEMU also
# supports the AN547 without the NPU.
#
# Default emulation:
#     QEMU is used by default for AN547 and
#     FVP  is used by default for AN552.
#
# To explicitly use FVP for emulation:
#     AN552 and AN547 can also use FVP as an emulation with
#     the 'run_armfvp' target, for example:
#
#     $ west build -b mps3//an547 samples/hello_world -t run_armfvp


if(CONFIG_BOARD_MPS3_CORSTONE300_AN547 OR CONFIG_BOARD_MPS3_CORSTONE300_AN547_NS)
  set(SUPPORTED_EMU_PLATFORMS qemu armfvp)
  set(ARMFVP_BIN_NAME FVP_Corstone_SSE-300_Ethos-U55)

  # QEMU settings
  set(QEMU_CPU_TYPE_${ARCH} cortex-m55)
  set(QEMU_FLAGS_${ARCH}
    -cpu ${QEMU_CPU_TYPE_${ARCH}}
    -machine mps3-an547
    -nographic
    -vga none
    )
elseif(CONFIG_BOARD_MPS3_CORSTONE300_AN552 OR CONFIG_BOARD_MPS3_CORSTONE300_AN552_NS)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_Corstone_SSE-300_Ethos-U55)
elseif(CONFIG_BOARD_MPS3_CORSTONE300)
  string(REPLACE "mps3/corstone300;" "" board_targets "${board_targets}")
  string(REPLACE ";" "\n" board_targets "${board_targets}")
  message(FATAL_ERROR "Please use a target from the list below: \n${board_targets}\n")
elseif(CONFIG_BOARD_MPS3_CORSTONE310_AN555 OR CONFIG_BOARD_MPS3_CORSTONE310_AN555_NS)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_Corstone_SSE-310)
  set(ARMFVP_FLAGS
    # default is '0x11000000' but should match cpu<i>.INITSVTOR which is 0.
    -C mps3_board.sse300.iotss3_systemcontrol.INITSVTOR_RST=0
  )
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
