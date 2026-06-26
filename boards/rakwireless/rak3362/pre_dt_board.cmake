# Copyright (c) 2026 RAKwireless Technology Limited
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - power@... & clock@... & regulators@...
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
