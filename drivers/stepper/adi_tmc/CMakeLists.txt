# SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC_SPI adi_tmc_spi.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC2209 adi_tmc22xx_stepper_controller.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC50XX adi_tmc50xx_stepper_controller.c)
