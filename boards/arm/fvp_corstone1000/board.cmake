# Copyright (c) 2026 BayLibre SAS
# Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS armfvp)
if(CONFIG_BOARD_FVP_CORSTONE1000_A320 OR CONFIG_BOARD_FVP_CORSTONE1000_A320_SMP)
  set(ARMFVP_BIN_NAME FVP_Corstone-1000-A320)
  # Set minimum FVP version for Corstone-1000-A320
  set(ARMFVP_MIN_VERSION 11.27.31)
elseif(CONFIG_BOARD_FVP_CORSTONE1000_CORSTONE1000_A35 OR CONFIG_BOARD_FVP_CORSTONE1000_CORSTONE1000_EXTSYS)
  set(ARMFVP_BIN_NAME FVP_Corstone-1000)
  # Set minimum FVP version for Corstone-1000
  set(ARMFVP_MIN_VERSION 11.23.25)
endif()

# Find the FVP binary. Board-specific HINTS supplement the global ARMFVP_BIN_PATH.
find_program(ARMFVP
  NAMES ${ARMFVP_BIN_NAME}
  HINTS
    ${ARMFVP_BIN_PATH}
    $ENV{ARMFVP_BIN_PATH}
)

# =============================================================================
# FVP Configuration
# =============================================================================

# CORSTONE1000_FIRMWARE_DIR is populated by sysbuild.cmake and propagated
# through the sysbuild cache to this image build.
zephyr_get(CORSTONE1000_FIRMWARE_DIR)

set(ARMFVP_FLAGS
  # --- Firmware loading ---
  -C se.trustedBootROMloader.fname=${CORSTONE1000_FIRMWARE_DIR}/bl1.bin
  --data board.flash0=${CORSTONE1000_FIRMWARE_DIR}/cs1000.bin@0x68000000

  # --- Terminal configuration ---
  # mode=raw is required on all terminals, otherwise zero output appears
  -C se.secenc_terminal.start_telnet=0
  -C se.secenc_terminal.mode=raw
  -C host.host_terminal_0.start_telnet=0
  -C host.host_terminal_0.mode=raw
  -C host.host_terminal_1.start_telnet=0
  -C host.host_terminal_1.mode=raw

  # --- UART output to console ---
  -C se.uart0.out_file=-
  -C se.uart0.unbuffered_output=1
  -C host.uart0.out_file=-
  -C host.uart0.unbuffered_output=1
  -C host.uart0.uart_enable=1
  -C host.uart1.out_file=-
  -C host.uart1.unbuffered_output=1

  # --- Hardware configuration ---
  -C board.xnvm_size=64
  -C se.trustedSRAM_config=6
  -C se.BootROM_config=3
  -C board.se_flash_size=8192
  -C se.nvm.update_raw_image=0
  -C se.cryptocell.USER_OTP_FILTERING_DISABLE=1
  -C board.smsc_91c111.enabled=0
  -C disable_visualisation=true
)

if(CONFIG_BOARD_FVP_CORSTONE1000_A320 OR CONFIG_BOARD_FVP_CORSTONE1000_A320_SMP)
  list(APPEND ARMFVP_FLAGS -C host.gic.ITS-count=0)
endif()

if(CONFIG_BOARD_FVP_CORSTONE1000_CORSTONE1000_EXTSYS)
  list(APPEND ARMFVP_FLAGS
    # --- Firmware loading ---
    -C extsys_harness0.extsys_flashloader.fname=${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin

    # --- Terminal configuration ---
    # mode=raw is required, otherwise zero output appears
    -C extsys0.extsys_terminal.start_telnet=0
    -C extsys0.extsys_terminal.mode=raw

    # --- UART output to console ---
    -C extsys0.uart0.out_file=-
    -C extsys0.uart0.unbuffered_output=1

    # --- Disable Cortex-A35 UART output ---
    -C host.uart0.out_file=/dev/null
    -C host.uart1.out_file=/dev/null

    # --- Redirect Secure Enclave UART output ---
    -C se.uart0.out_file=${APPLICATION_BINARY_DIR}/secure-enclave.log
  )
endif()
