# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_AGS10 ags10)
add_subdirectory_ifdef(CONFIG_DHT dht)
add_subdirectory_ifdef(CONFIG_DHT20 dht20)
# zephyr-keep-sorted-stop
