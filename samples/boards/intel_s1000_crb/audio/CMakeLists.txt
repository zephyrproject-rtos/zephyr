# SPDX-License-Identifier: Apache-2.0

set(BOARD intel_s1000_crb)
cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/audio_driver.c)
target_sources(app PRIVATE src/audio_core.c)
target_sources(app PRIVATE src/audio_proc.c)
target_sources(app PRIVATE src/tuning_driver.c)
target_sources(app PRIVATE src/usb_transport.c)
target_sources(app PRIVATE src/framework.c)
