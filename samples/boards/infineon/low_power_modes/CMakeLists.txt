# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(low_power_modes)

target_sources(app PRIVATE src/main.c)

# The DUT drives the test sequence and its peripheral self-tests; the companion
# is the second-core mailbox receiver.  Exactly one role is built per image.
target_sources_ifdef(CONFIG_APP_ROLE_DUT app PRIVATE src/main_dut.c)
target_sources_ifdef(CONFIG_APP_ROLE_DUT app PRIVATE src/peripherals.c)
target_sources_ifdef(CONFIG_APP_ROLE_COMPANION app PRIVATE src/main_companion.c)

# The inter-core mailbox is needed by the companion (receiver) and by a DUT that
# commands a companion (sender).  Single-core DUTs omit it entirely.
if(CONFIG_APP_ROLE_COMPANION OR CONFIG_APP_COMPANION_MBOX)
  target_sources(app PRIVATE src/ds_mbox.c)
endif()
