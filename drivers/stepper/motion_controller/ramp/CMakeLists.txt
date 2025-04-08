# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_STEPPER_RAMP_CONSTANT stepper_ramp_constant.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_RAMP_TRAPEZOIDAL stepper_ramp_trapezoidal.c)
