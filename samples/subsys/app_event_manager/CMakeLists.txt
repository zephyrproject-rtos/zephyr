#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Event Manager sample")

# Include application event headers
zephyr_library_include_directories(src/events)

# Application sources
# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE
		src/events/ack_event.c
		src/events/config_event.c
		src/events/control_event.c
		src/events/measurement_event.c
)

target_sources(app PRIVATE
		src/modules/controller.c
		src/modules/sensor_simulated.c
		src/modules/stats.c
)
# NORDIC SDK APP END
