# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hci_ipc)

target_sources(app PRIVATE src/main.c)

# Remove after 3.7.0 is released
dt_chosen(chosen_hci_rpmsg PROPERTY "zephyr,bt-hci-rpmsg-ipc")
if(DEFINED chosen_hci_rpmsg)
  message(FATAL_ERROR  "zephyr,bt-hci-rpmsg-ipc has been renamed to zephyr,bt-hci-ipc")
endif()
