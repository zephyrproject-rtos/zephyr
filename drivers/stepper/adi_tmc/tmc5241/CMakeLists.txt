# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC5241                tmc5241.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC5241_STEPPER_DRIVER tmc5241_stepper_driver.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC5241_STEPPER_CTRL   tmc5241_stepper_ctrl.c)
