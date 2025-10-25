# SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC51XX tmc51xx.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC51XX_MOTION_CONTROLLER tmc51xx_motion_controller.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC51XX_STEPPER_DRIVER tmc51xx_stepper_driver.c)
