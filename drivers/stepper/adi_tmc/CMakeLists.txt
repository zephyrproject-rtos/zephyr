# SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC2209 tmc22xx.c)
zephyr_library_sources_ifdef(CONFIG_ADI_TMC5XXX_STEPPER_DRIVER tmc5xxx_stepper_driver.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC51XX tmc51xx.c)
