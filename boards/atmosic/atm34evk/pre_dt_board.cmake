# Copyright (c) 2024, Atmosic
#
# SPDX-License-Identifier: Apache-2.0
#

# Suppress DTC warnings due to computed image partitions will not match unit-addresses
# see: slot1_partition: partition@cece0040 {
# the slot1 partition location is computed based on storage size, SPE_SIZE, RUN_IN_FLASH usage etc..
#
list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")
