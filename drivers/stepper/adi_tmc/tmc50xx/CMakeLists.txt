# SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(tmc50xx.c)
zephyr_library_sources_ifdef(CONFIG_TMC50XX_STEPPER tmc50xx_stepper.c)
zephyr_library_sources_ifdef(CONFIG_TMC50XX_STEPPER_DRV tmc50xx_stepper_drv.c)
