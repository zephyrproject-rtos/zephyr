# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_BT_SHELL shell)

zephyr_sources_ifdef(
        CONFIG_BT_IAS
        ias.c)

zephyr_library_sources_ifdef(
        CONFIG_BT_IAS_CLIENT
        ias_client.c)
