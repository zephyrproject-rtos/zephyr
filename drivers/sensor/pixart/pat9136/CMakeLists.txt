# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
	pat9136.c
	pat9136_decoder.c
)

zephyr_library_sources_ifdef(CONFIG_PAT9136_STREAM
	pat9136_stream.c
)
