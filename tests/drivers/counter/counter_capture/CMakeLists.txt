# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(counter_capture)

target_sources(app PRIVATE src/capture_test.c)

# Select the trigger-source backend: the emulated counter on native_sim, a PWM
# looped back to the capture pad when the board enables PWM and declares
# "pwms" on /zephyr,user (no external wire), or a GPIO wired to the capture
# pin on other real hardware.
if(CONFIG_COUNTER_NATIVE_SIM)
  target_sources(app PRIVATE src/counter_capture_emul_src.c)
elseif(CONFIG_PWM)
  target_sources(app PRIVATE src/pwm_src.c)
else()
  target_sources(app PRIVATE src/gpio_src.c)
endif()
