# SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(stepper_event_handler.c)
zephyr_linker_sources(DATA_SECTIONS iterables.ld)
