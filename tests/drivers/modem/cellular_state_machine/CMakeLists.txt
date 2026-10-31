# Copyright (c) 2026 Embeint Inc
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_cellular_state_machine_test)

target_sources(app PRIVATE
  driver/modem_cellular_test.c
  src/common/stubs.c
  src/common/test_helpers.c
)
target_include_directories(app PRIVATE
  driver
  src/common
)

# On-demand connect rejects modems with a network script, so it runs on its own devicetree
if(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT)
  target_sources(app PRIVATE
    src/on_demand.c
  )
else()
  target_sources(app PRIVATE
    src/api.c
    src/board_init.c
    src/gpio.c
    src/main.c
    src/modem_info.c
    src/network_status.c
    src/periodic.c
    src/ppp_dead.c
    src/ring.c
    src/script_errors.c
    src/signal.c
    src/suspend.c
    src/unsolicited.c
  )
endif()
