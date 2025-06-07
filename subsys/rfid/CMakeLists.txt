# Copyright (c) 2025 Conny Marco Menebröcker
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RFID_ISO14443A rfid_iso14443a.c)
