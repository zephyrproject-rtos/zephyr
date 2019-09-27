# SPDX-License-Identifier: Apache-2.0

set(BOARD intel_s1000_crb)
cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/dmic_sample.c)
