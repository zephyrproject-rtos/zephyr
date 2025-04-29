# Copyright (c) 2025 Croxel, Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
	rm3100.c
	rm3100_decoder.c
)
zephyr_library_sources_ifdef(CONFIG_RM3100_STREAM
	rm3100_stream.c
)
