# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_BD8LB600FS_DIAGNOSTICS bd8lb600fs)
add_subdirectory_ifdef(CONFIG_BH1730 bh1730)
add_subdirectory_ifdef(CONFIG_BH1750 bh1750)
add_subdirectory_ifdef(CONFIG_BH1790 bh1790)
# zephyr-keep-sorted-stop
