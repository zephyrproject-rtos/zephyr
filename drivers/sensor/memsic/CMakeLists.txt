# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Intel

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_MC3419 mc3419)
add_subdirectory_ifdef(CONFIG_MMC56X3 mmc56x3)
# zephyr-keep-sorted-stop
