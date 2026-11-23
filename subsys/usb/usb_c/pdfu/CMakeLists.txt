# Copyright (c) 2026 Meta Platforms, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_USBC_PDFU_RESPONDER pdfu_responder.c)
