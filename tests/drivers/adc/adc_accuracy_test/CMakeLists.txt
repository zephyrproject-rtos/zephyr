# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dac_accuracy)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_REFERENCE_VOLTAGE_TEST app PRIVATE src/ref_volt.c)
target_sources_ifdef(CONFIG_DAC_SOURCE_TEST app PRIVATE src/dac_source.c)
