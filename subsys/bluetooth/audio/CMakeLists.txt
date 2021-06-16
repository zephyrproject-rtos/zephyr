# SPDX-License-Identifier: Apache-2.0

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
