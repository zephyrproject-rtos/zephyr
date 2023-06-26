# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_bt_host_settings_cleanup)

target_sources(app PRIVATE
  src/main.c
  src/test.c
)

zephyr_include_directories(
        $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
        $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/

        $ENV{ZEPHYR_BASE}/subsys/bluetooth/host/
)
