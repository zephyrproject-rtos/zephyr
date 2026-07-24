# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0
add_subdirectory_ifdef(CONFIG_UWB uci_lib)
add_subdirectory_ifdef(CONFIG_UWB uwb_api)
