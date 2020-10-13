# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_CCID ccid.c)
zephyr_library_sources_ifdef(CONFIG_BT_CSIS csis.c)
zephyr_library_sources_ifdef(CONFIG_BT_CSIP csip.c)
if (CONFIG_BT_CSIS OR CONFIG_BT_CSIP)
	zephyr_library_sources(sih.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_BASS bass.c)
zephyr_library_sources_ifdef(CONFIG_BT_BASS_CLIENT bass_client.c)
zephyr_library_sources_ifdef(CONFIG_BT_TBS tbs.c)
zephyr_library_sources_ifdef(CONFIG_BT_CCP ccp.c)
zephyr_library_sources_ifdef(CONFIG_BT_PACS pacs.c)
