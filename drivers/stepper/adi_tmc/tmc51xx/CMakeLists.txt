# SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(tmc51xx.c)
zephyr_library_sources_ifdef(CONFIG_TMC51XX_STEPPER tmc51xx_stepper.c)
zephyr_library_sources_ifdef(CONFIG_TMC51XX_STEPPER_DRV tmc51xx_stepper_drv.c)
