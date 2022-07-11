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

if (CONFIG_BT_MICP_MIC_DEV)
	zephyr_library_sources(micp_mic_dev.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_MICP_MIC_CTLR micp_mic_ctlr.c)

zephyr_library_sources_ifdef(CONFIG_BT_CCID ccid.c)

zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_CSIS csis.c)
zephyr_library_sources_ifdef(CONFIG_BT_CSIS_CLIENT csis_client.c)
if (CONFIG_BT_CSIS OR CONFIG_BT_CSIS_CLIENT)
	zephyr_library_sources(csis_crypto.c)
endif()

zephyr_library_sources_ifdef(CONFIG_BT_TBS tbs.c)

zephyr_library_sources_ifdef(CONFIG_BT_TBS_CLIENT tbs_client.c)

zephyr_library_sources_ifdef(CONFIG_BT_MCC mcc.c)

zephyr_library_sources_ifdef(CONFIG_BT_MCS mcs.c)

zephyr_library_sources_ifdef(CONFIG_BT_MPL mpl.c)

zephyr_library_sources_ifdef(CONFIG_MCTL media_proxy.c)

zephyr_library_sources_ifdef(CONFIG_BT_ASCS ascs.c)
zephyr_library_sources_ifdef(CONFIG_BT_PACS pacs.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_STREAM stream.c codec.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_UNICAST_SERVER unicast_server.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_CAPABILITY capabilities.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_UNICAST_CLIENT unicast_client.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_BROADCAST_SOURCE broadcast_source.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_BROADCAST_SINK broadcast_sink.c)
zephyr_library_sources_ifdef(CONFIG_BT_BASS bass.c)
zephyr_library_sources_ifdef(CONFIG_BT_BASS_CLIENT bass_client.c)
zephyr_library_sources_ifdef(CONFIG_BT_HAS has.c)
zephyr_library_sources_ifdef(CONFIG_BT_HAS_CLIENT has_client.c)
zephyr_library_sources_ifdef(CONFIG_BT_CAP cap.c)
