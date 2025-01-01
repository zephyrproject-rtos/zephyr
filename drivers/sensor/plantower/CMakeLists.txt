# Copyright (c) 2025 Alex Bucknall
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_PMS7003 pms7003)
add_subdirectory_ifdef(CONFIG_PMSA003I pmsa003i)
# zephyr-keep-sorted-stop
