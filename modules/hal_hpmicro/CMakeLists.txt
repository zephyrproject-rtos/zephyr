# Copyright (c) 2022 HPMicro
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_HPMICRO_HAL)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} hal_hpmicro)
endif()
