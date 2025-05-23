# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# Copyright (c) 2025 Paul Timke <ptimkec@live.com>
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_PAA3905 paa3905)
add_subdirectory_ifdef(CONFIG_PAJ7620 paj7620)
add_subdirectory_ifdef(CONFIG_PAT9136 pat9136)
# zephyr-keep-sorted-stop
