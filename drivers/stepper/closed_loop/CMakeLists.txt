# Copyright (c) 2026 Contributors to the Zephyr Project
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../event_handler/include)

zephyr_library_sources_ifdef(CONFIG_STEPPER_CLOSED_LOOP stepper_closed_loop.c)
zephyr_library_sources_ifdef(CONFIG_FAKE_ENCODER fake_encoder.c)
