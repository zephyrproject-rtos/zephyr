# Copyright (c) 2026 Dotcom IoT LLP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(ams_as7341.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_AMS_AS7341 ams_as7341_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_AMS_AS7341 .)
