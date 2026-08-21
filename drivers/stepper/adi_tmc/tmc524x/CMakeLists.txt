# SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC524X tmc524x.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC524X_STEPPER_CTRL tmc524x_stepper_ctrl.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC524X_STEPPER_DRIVER tmc524x_stepper_driver.c)
