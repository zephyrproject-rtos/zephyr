# Copyright (c) 2022 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

# Add abstraction-rtos
zephyr_include_directories(${ZEPHYR_HAL_INFINEON_MODULE_DIR}/abstraction-rtos/include)

# Enable Zephyr support
zephyr_include_directories_ifdef(CONFIG_ABSTRACTION_RTOS_COMPONENT_ZEPHYR
        include/COMPONENT_ZEPHYR)
zephyr_library_sources_ifdef(CONFIG_ABSTRACTION_RTOS_COMPONENT_ZEPHYR
        source/COMPONENT_ZEPHYR/cyabs_rtos_zephyr.c)
