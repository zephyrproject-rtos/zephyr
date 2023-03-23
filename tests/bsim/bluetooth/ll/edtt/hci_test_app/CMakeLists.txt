# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Oticon A/S

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hci_test_app)

target_sources(app PRIVATE
	src/main.c
	../common/edtt_driver_bsim.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/samples/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth
  ../common/
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
