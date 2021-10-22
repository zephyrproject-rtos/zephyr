# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_BT_VOCS OR CONFIG_BT_VOCS_CLIENT)
	zephyr_library_sources(vocs.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_VOCS_CLIENT vocs_client.c)

if (CONFIG_BT_AICS OR CONFIG_BT_AICS_CLIENT)
	zephyr_library_sources(aics.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_AICS_CLIENT aics_client.c)

if (CONFIG_BT_VCS OR CONFIG_BT_VCS_CLIENT)
	zephyr_library_sources(vcs.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_VCS_CLIENT vcs_client.c)

if (CONFIG_BT_MICS OR CONFIG_BT_MICS_CLIENT)
	zephyr_library_sources(mics.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_MICS_CLIENT mics_client.c)

zephyr_library_sources_ifdef(CONFIG_BT_CCID ccid.c)

zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_CSIS csis.c)
zephyr_library_sources_ifdef(CONFIG_BT_CSIS_CLIENT csis_client.c)
if (CONFIG_BT_CSIS OR CONFIG_BT_CSIS_CLIENT)
	zephyr_library_sources(csis_crypto.c)
endif()

zephyr_library_sources_ifdef(CONFIG_BT_OTC otc.c)

zephyr_library_sources_ifdef(CONFIG_BT_MCC mcc.c)

zephyr_library_sources_ifdef(CONFIG_BT_MCS mcs.c)

zephyr_library_sources_ifdef(CONFIG_BT_MCS mpl.c)

zephyr_library_sources_ifdef(CONFIG_BT_MCS media_proxy.c)
