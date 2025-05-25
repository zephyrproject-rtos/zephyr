# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_TACH_IT51XXX ite_tach_it51xxx)
add_subdirectory_ifdef(CONFIG_TACH_IT8XXX2 ite_tach_it8xxx2)
add_subdirectory_ifdef(CONFIG_VCMP_IT8XXX2 ite_vcmp_it8xxx2)
# zephyr-keep-sorted-stop
