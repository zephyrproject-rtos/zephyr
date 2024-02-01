# SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_TMC5041_SPI    tmc5041_spi.c)
zephyr_library_sources_ifdef(CONFIG_TMC5041    tmc5041.c)
zephyr_library_sources_ifdef(CONFIG_TMC5041_SHELL  tmc5041_shell.c)
zephyr_library_sources_ifdef(CONFIG_TMC5041_STEPPER_MOTOR_DEVICE tmc5041_stepper_motor_device.c)
zephyr_library_sources_ifdef(CONFIG_TMC5041_STEPPER_MOTOR_DEVICE_SHELL tmc5041_stepper_motor_device_shell.c)
