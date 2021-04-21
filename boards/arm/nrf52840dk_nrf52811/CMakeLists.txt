# Copyright (c) 2019 Nordic Semiconductor ASA

# SPDX-License-Identifier: Apache-2.0

# The nrf52840dk_nrf52811 board mirrors the nRF52840 DK hardware. This
# needs to be considered by certain system initialization functionality
# residing in system_nrf52811.c and SoC dependent routines in nrfx_coredep.h.
zephyr_compile_definitions(DEVELOP_IN_NRF52840)
zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)
