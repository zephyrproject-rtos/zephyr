# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(maxm86161.c)
zephyr_library_sources_ifdef(CONFIG_MAXM86161_TRIGGER maxm86161_trigger.c)
zephyr_library_sources_ifdef(CONFIG_MAXM86161_STREAM maxm86161_decoder.c maxm86161_rtio.c)
zephyr_library_sources_ifdef(CONFIG_MAXM86161_STREAM maxm86161_stream.c)
