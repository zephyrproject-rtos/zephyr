# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(stepper_work_timing.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_TIMING_SOURCES_COUNTER_TIMING stepper_counter_timing.c)
