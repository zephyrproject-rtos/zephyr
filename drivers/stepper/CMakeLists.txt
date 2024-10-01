# SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/stepper.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_FAKE_STEPPER fake_stepper_controller.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_STEPPER gpio_stepper_controller.c)

zephyr_library_sources_ifdef(CONFIG_STEPPER_SHELL stepper_shell.c)
