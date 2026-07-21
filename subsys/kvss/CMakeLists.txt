# Copyright (c) 2026 Nordic Semiconductor (ASA)
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_NVS  ./nvs)
add_subdirectory_ifdef(CONFIG_ZMS  ./zms)
