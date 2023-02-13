# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_FLASH_MAP  flash_map)
add_subdirectory_ifdef(CONFIG_STREAM_FLASH stream)
