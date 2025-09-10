# Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

#
# Default emulation:
#     FVP  is used by default for corstone320/fvp.
#


if(CONFIG_BOARD_MPS4_CORSTONE315_FVP OR CONFIG_BOARD_MPS4_CORSTONE315_FVP_NS)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_Corstone_SSE-315)
elseif(CONFIG_BOARD_MPS4_CORSTONE320_FVP OR CONFIG_BOARD_MPS4_CORSTONE320_FVP_NS)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_Corstone_SSE-320)
endif()

if(CONFIG_BOARD_MPS4_CORSTONE315_FVP OR CONFIG_BOARD_MPS4_CORSTONE320_FVP)
    set(ARMFVP_FLAGS
      # default is '0x11000000' but should match cpu<i>.INITSVTOR which is 0.
      -C mps4_board.subsystem.iotss3_systemcontrol.INITSVTOR_RST=0
      # default is 0x8, this change is needed since we split flash into itcm
      # and sram and it reduces the number of available mpu regions causing a
      # few MPU tests to fail.
      -C mps4_board.subsystem.cpu0.MPU_S=16
    )
endif()

if(CONFIG_ARM_PAC OR CONFIG_ARM_BTI)
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    -C mps4_board.subsystem.cpu0.CFGPACBTI=1
  )
endif()

if(CONFIG_BUILD_WITH_TFM)
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS} -a ${APPLICATION_BINARY_DIR}/zephyr/tfm_merged.hex)
endif()

# FVP Parameters
# -C indicate a config option in the form of:
#   instance.parameter=value
# Run the FVP with --list-params to list all options
set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
  -C mps4_board.uart0.out_file=-
  -C mps4_board.uart0.unbuffered_output=1
  -C mps4_board.uart1.out_file=-
  -C mps4_board.uart1.unbuffered_output=1
  -C mps4_board.uart2.out_file=-
  -C mps4_board.uart2.unbuffered_output=1
  -C mps4_board.visualisation.disable-visualisation=1
  -C mps4_board.telnetterminal0.start_telnet=0
  -C mps4_board.telnetterminal1.start_telnet=0
  -C mps4_board.telnetterminal2.start_telnet=0
  -C vis_hdlcd.disable_visualisation=1
  )
