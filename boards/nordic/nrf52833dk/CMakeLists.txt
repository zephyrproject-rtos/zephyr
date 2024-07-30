# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF52833DK_NRF52820)
  # The nrf52833dk_nrf52820 board mirrors the nRF52833 DK hardware. This
  # needs to be considered by certain system initialization functionality
  # residing in system_nrf52820.c and SoC dependent routines in nrfx_coredep.h.
  zephyr_compile_definitions(DEVELOP_IN_NRF52833)
  zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)
endif()
