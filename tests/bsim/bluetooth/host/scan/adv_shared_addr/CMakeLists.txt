# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_scan_adv_shared_addr)

target_sources(app PRIVATE
  src/main.c
)

# The test records the HCI commands the host sends while scanning starts.
target_link_options(app PUBLIC -Wl,--wrap=bt_hci_cmd_send_sync)

# This contains babblesim-specific helpers, e.g. device synchronization.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
        $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
        $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
