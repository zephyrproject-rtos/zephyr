# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_CCID ccid.c)
zephyr_library_sources_ifdef(CONFIG_BT_CSIS csis.c)
zephyr_library_sources_ifdef(CONFIG_BT_CSIP csip.c)
if (CONFIG_BT_CSIS OR CONFIG_BT_CSIP)
	zephyr_library_sources(csis_crypto.c)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_BASS bass.c)
zephyr_library_sources_ifdef(CONFIG_BT_BASS_CLIENT bass_client.c)
zephyr_library_sources_ifdef(CONFIG_BT_TBS tbs.c)
zephyr_library_sources_ifdef(CONFIG_BT_CCP ccp.c)
zephyr_library_sources_ifdef(CONFIG_BT_PACS pacs.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_STREAM stream.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_BROADCAST_SOURCE broadcast_source.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_BROADCAST_SINK broadcast_sink.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_ENDPOINT endpoint.c)
zephyr_library_sources_ifdef(CONFIG_BT_AUDIO_CAPABILITIES capabilities.c)
zephyr_library_sources_ifdef(CONFIG_BT_ASCS ascs.c)
zephyr_library_sources_ifdef(CONFIG_BT_BAP bap.c)
zephyr_library_sources_ifdef(CONFIG_BT_MCS mcs.c)
zephyr_library_sources_ifdef(CONFIG_BT_MCS media_proxy.c)
zephyr_library_sources_ifdef(CONFIG_BT_MCS mpl.c)
zephyr_library_sources_ifdef(CONFIG_BT_MCC mcc.c)
zephyr_library_sources_ifdef(CONFIG_BT_OTC otc.c)

if(CONFIG_BT_CSIS_TEST_SAMPLE_DATA)
  message(WARNING "CONFIG_BT_CSIS_TEST_SAMPLE_DATA is enabled.
  Using the CSIS Test Sample Data will make the SIRK, the advertised value and
  the LTK used to encrypt the SIRK constant and be public data from the CSIS
  specification. Do not use in a production build."
    )
endif()
if(CONFIG_BT_CSIP_TEST_SAMPLE_DATA)
  message(WARNING "CONFIG_BT_CSIP_TEST_SAMPLE_DATA is enabled.
  Using the CSIS Test Sample Data will make the client only work with set members
  using the same sample data. Do not use in a production build."
    )
endif()
