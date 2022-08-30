# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_EDAC_IBECC edac_ibecc.c)
zephyr_library_sources_ifdef(CONFIG_EDAC_SHELL shell.c)
