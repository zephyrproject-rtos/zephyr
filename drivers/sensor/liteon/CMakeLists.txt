# Copyright (c) 2025 Konrad Sikora
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_LTR55X ltr55x)
add_subdirectory_ifdef(CONFIG_LTR_F216A ltrf216a)
# zephyr-keep-sorted-stop
