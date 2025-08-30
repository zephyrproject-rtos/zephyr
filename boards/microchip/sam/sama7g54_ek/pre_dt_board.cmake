# Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
#
# SPDX-License-Identifier: Apache-2.0
#

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - /soc/ethernet@e2800000 & /soc/mdio@e2800000
# - /soc/ethernet@e2804000 & /soc/mdio@e2804000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
