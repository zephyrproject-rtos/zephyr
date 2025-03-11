# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
	paa3905.c
	paa3905_decoder.c
)

zephyr_library_sources_ifdef(CONFIG_PAA3905_STREAM
	paa3905_stream.c
)
