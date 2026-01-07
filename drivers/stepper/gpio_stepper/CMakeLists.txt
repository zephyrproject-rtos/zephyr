# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_H_BRIDGE_STEPPER h_bridge_stepper.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_STEP_DIR_STEPPER gpio_step_dir.c)

add_subdirectory_ifdef(CONFIG_GPIO_STEPPER common)
