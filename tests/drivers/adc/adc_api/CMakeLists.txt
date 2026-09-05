# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(adc_api)

target_sources(app PRIVATE src/channels.c)
target_sources_ifdef(CONFIG_TEST_ADC_BASIC app PRIVATE src/main.c src/test_adc.c)
target_sources_ifdef(CONFIG_TEST_ADC_ERROR_CASES app PRIVATE src/test_error_cases.c)
target_sources_ifdef(CONFIG_TEST_ADC_ACCURACY_REF app PRIVATE src/test_accuracy_ref.c)
target_sources_ifdef(CONFIG_TEST_ADC_ACCURACY_DAC app PRIVATE src/test_accuracy_dac.c)
