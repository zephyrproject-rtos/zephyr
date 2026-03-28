# SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC50XX tmc50xx.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC50XX_STEPPER_CTRL tmc50xx_stepper_ctrl.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC50XX_STEPPER_DRIVER tmc50xx_stepper_driver.c)
