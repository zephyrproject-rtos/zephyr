# Copyright (c) 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - power@X010e000 & clock@X010e000 & xo@X010e000 & lfclk@X010e000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
