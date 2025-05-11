# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC_SPI adi_tmc_spi.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC50XX tmc50xx.c tmc50xx_stepper_control.c)
