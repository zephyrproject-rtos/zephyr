# Copyright (c) 2026 Anuj Deshpande
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_HAPTICS_TM6605 tm6605.c)
