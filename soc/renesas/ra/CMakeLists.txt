# Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
# SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_HAS_RENESAS_RA_FSP common)
add_subdirectory(${CONFIG_SOC_SERIES})
