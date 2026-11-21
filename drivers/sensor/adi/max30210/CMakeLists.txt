# Copyright (c) 2025 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(max30210.c)
zephyr_library_sources_ifdef(CONFIG_MAX30210_TRIGGER max30210_trigger.c)
zephyr_library_sources_ifdef(CONFIG_MAX30210_STREAM max30210_stream.c max30210_decoder.c)
