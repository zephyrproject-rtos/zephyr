# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_BT_SHELL shell)

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_A2DP             a2dp.c a2dp_codec_sbc.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVDTP            avdtp.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVRCP            avrcp.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVCTP            avctp.c)
zephyr_library_sources_ifdef(CONFIG_BT_RFCOMM           rfcomm.c)

zephyr_library_sources_ifdef(
  CONFIG_BT_CLASSIC
  br.c
  conn_br.c
  keys_br.c
  l2cap_br.c
  sdp.c
  ssp.c
  sco.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_HFP_HF
  hfp_hf.c
  at.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_HFP_AG
  hfp_ag.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_GOEP
  obex.c
  goep.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_DID
  did.c
  )
