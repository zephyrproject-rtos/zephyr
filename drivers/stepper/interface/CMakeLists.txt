# SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_STEPPER_INTERFACE_STEP_DIR stepper_interface_step_dir.c)
