# Copyright 2024 Croxel, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_ext_adv)

target_sources(app PRIVATE
    src/common.c
)


target_sources_ifdef(CONFIG_BT_CENTRAL app PRIVATE
    src/ext_adv_scanner.c
)

target_sources_ifdef(CONFIG_BT_PERIPHERAL app PRIVATE
    src/ext_adv_advertiser.c
)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
