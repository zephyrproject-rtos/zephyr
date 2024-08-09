# SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_GPIO_STEPPER gpio_stepper)

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/stepper.h)

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_STEPPER_SHELL stepper_shell.c)
