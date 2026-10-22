# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF52DK_NRF52805 OR CONFIG_BOARD_NRF52DK_NRF52810)
  # The nrf52dk_nrf52805/nrf52dk_nrf52810 boards mirrors the nRF52832 DK hardware. This
  # needs to be considered by certain system initialization functionality
  # residing in system_nrf52810.c and SoC dependent routines in nrfx_coredep.h.
  zephyr_compile_definitions(DEVELOP_IN_NRF52832)
  zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)
endif()
