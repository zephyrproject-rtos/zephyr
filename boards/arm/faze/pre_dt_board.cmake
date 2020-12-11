# Copyright (c) 2019, NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress DTC warnings due to all GPIO nodes sharing the same register address.
list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")
