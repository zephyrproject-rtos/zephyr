# SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(tmcm3216.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMCM3216_STEPPER_DRIVER tmcm3216_stepper_driver.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMCM3216_STEPPER_CTRL tmcm3216_stepper_ctrl.c)
