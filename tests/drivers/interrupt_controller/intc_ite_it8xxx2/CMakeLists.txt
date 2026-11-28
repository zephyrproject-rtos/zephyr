# Copyright 2026 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(intc_ite_it8xxx2)

target_include_directories(app PRIVATE
	include
)

zephyr_include_directories(
	include
	${ZEPHYR_BASE}/soc/ite/ec/common
	${ZEPHYR_BASE}/soc/ite/ec/it8xxx2
)

target_sources(app PRIVATE
	src/main.c
)

if(CONFIG_TEST_IT8XXX2_INTC_V1)
	zephyr_compile_definitions(CONFIG_SOC_IT8XXX2_REG_SET_V1=1)
	target_sources(app PRIVATE
		${ZEPHYR_BASE}/drivers/interrupt_controller/intc_ite_it8xxx2.c
	)
elseif(CONFIG_TEST_IT8XXX2_INTC_V2)
	zephyr_compile_definitions(CONFIG_SOC_IT8XXX2_REG_SET_V2=1)
	target_sources(app PRIVATE
		${ZEPHYR_BASE}/drivers/interrupt_controller/intc_ite_it8xxx2_v2.c
	)
endif()
