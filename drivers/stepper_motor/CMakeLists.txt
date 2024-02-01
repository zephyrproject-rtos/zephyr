# SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_TMC5041 tmc5041)

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/stepper_motor_controller.h)
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/stepper_motor_device.h)

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
