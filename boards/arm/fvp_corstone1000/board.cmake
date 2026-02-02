# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS armfvp)
set(ARMFVP_BIN_NAME FVP_Corstone-1000-A320)

# Find the FVP binary. Board-specific HINTS supplement the global ARMFVP_BIN_PATH.
find_program(ARMFVP
  NAMES ${ARMFVP_BIN_NAME}
  HINTS
    ${ARMFVP_BIN_PATH}
    $ENV{ARMFVP_BIN_PATH}
)

# Set minimum FVP version for Corstone-1000-A320
set(ARMFVP_MIN_VERSION 11.27.31)

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
  -C host.gic.ITS-count=0
  -C board.smsc_91c111.enabled=0
  -C disable_visualisation=true
)
