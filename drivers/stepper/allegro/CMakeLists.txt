# SPDX-FileCopyrightText: Copyright (c) 2025 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_A4979_STEPPER a4979.c)
