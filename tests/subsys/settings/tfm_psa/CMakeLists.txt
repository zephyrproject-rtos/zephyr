# Copyright (c) 2025 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_settings_psa)

add_subdirectory(./src psa_test_bindir)
