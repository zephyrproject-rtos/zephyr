# SPDX-FileCopyrightText: Copyright 2026 Ezurio LLC
# SPDX-License-Identifier: Apache-2.0

# Shared LE Audio playback helper for the samples under
# samples/bluetooth/audio/. Consuming samples add this directory via
# add_subdirectory() and include <sample_bt_audio_playback.h>.

zephyr_include_directories(include)
target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/audio_playback.c)
