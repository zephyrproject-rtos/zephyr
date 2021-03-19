# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

if (CONFIG_BT_VOCS OR CONFIG_BT_VOCS_CLIENT)
	zephyr_library_sources(vocs.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_VOCS_CLIENT vocs_client.c)
