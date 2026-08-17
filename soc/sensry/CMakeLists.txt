# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 sensry.io

zephyr_include_directories(${CONFIG_SOC_FAMILY}/${CONFIG_SOC_SERIES}/common)

add_subdirectory_ifdef(CONFIG_SOC_SERIES_SY1XX ganymed/sy1xx)
