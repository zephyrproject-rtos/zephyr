# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_A2DP             a2dp.c)
zephyr_library_sources_ifdef(CONFIG_BT_AVDTP            avdtp.c)
zephyr_library_sources_ifdef(CONFIG_BT_RFCOMM           rfcomm.c)

zephyr_library_sources_ifdef(
  CONFIG_BT_BREDR
  br.c
  keys_br.c
  l2cap_br.c
  sdp.c
  ssp.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_HFP_HF
  hfp_hf.c
  at.c
  )
