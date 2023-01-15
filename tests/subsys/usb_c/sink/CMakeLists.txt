# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sink_test)

target_sources(app PRIVATE
		src/main.c
		../mocks/mock_tcpc.c
		../mocks/mock_tester.c
		../mocks/mock_vbus_adc.c)
