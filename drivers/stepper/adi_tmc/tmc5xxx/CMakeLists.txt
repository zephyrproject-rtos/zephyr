# SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(adi_tmc5xxx_core.c)

# Common bus implementation sources
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC_SPI tmc5xxx_spi.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC_UART tmc5xxx_uart.c)

# Device-specific implementations
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC50XX tmc50xx.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC51XX tmc51xx.c)

