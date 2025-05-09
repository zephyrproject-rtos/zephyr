# SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC_SPI adi_tmc_spi.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC_UART adi_tmc_uart.c)
