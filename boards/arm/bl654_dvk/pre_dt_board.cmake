# Copyright (c) 2021 Linaro Limited
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - /soc/i2c@40003000 & /soc/spi@40003000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
