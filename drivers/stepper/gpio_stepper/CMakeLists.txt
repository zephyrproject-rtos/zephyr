# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_H_BRIDGE_STEPPER h_bridge_stepper.c)

add_subdirectory_ifdef(CONFIG_STEP_DIR_STEPPER step_dir)
