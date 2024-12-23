# SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(step_dir_stepper_common.c)
zephyr_library_sources(step_dir_stepper_work_timing.c)
zephyr_library_sources_ifdef(CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING step_dir_stepper_counter_timing.c)
