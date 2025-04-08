# SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(timing_source)
add_subdirectory_ifdef(CONFIG_STEPPER_RAMP ramp)

zephyr_library_sources(stepper_motion_controller.c)
