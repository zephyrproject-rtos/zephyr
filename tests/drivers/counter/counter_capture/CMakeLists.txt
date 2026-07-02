# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(counter_capture)

target_sources(app PRIVATE src/capture_test.c)

# Select the trigger-source backend: the emulated counter on native_sim, or a
# GPIO wired (loopback) to the capture pin on real hardware.
if(CONFIG_COUNTER_NATIVE_SIM)
  target_sources(app PRIVATE src/counter_capture_emul_src.c)
else()
  target_sources(app PRIVATE src/gpio_src.c)
endif()
