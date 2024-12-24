# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/audio)

if(CONFIG_BT_VCP_VOL_REND)
  target_sources(app PRIVATE btp_vcp.c)
endif()

if(CONFIG_BT_BAP_UNICAST OR BT_BAP_BROADCAST_SOURCE OR BT_BAP_BROADCAST_SINK)
    target_sources(app PRIVATE btp_bap_audio_stream.c btp_bap.c )
endif()

if(CONFIG_BT_BAP_UNICAST)
    target_sources(app PRIVATE btp_bap_unicast.c)
endif()

if(CONFIG_BT_BAP_BROADCAST_SOURCE OR CONFIG_BT_BAP_BROADCAST_SOURCE)
    target_sources(app PRIVATE btp_bap_broadcast.c)
endif()

if(CONFIG_BT_HAS)
    target_sources(app PRIVATE btp_has.c)
endif()

if (CONFIG_BT_CSIP_SET_MEMBER)
    target_sources(app PRIVATE btp_csis.c)
endif()

if (CONFIG_BT_CSIP_SET_COORDINATOR)
    target_sources(app PRIVATE btp_csip.c)
endif()

if(CONFIG_BT_MICP_MIC_DEV)
    target_sources(app PRIVATE btp_micp.c)
endif()

if(CONFIG_BT_AICS)
    target_sources(app PRIVATE btp_aics.c)
endif()

if (CONFIG_BT_TBS_CLIENT)
    target_sources(app PRIVATE btp_ccp.c)
endif()

if (CONFIG_BT_CAP_ACCEPTOR)
    target_sources(app PRIVATE btp_cas.c)
endif()

if(CONFIG_BT_MCC OR CONFIG_BT_MCS)
    target_sources(app PRIVATE btp_mcp.c)
endif()

if(CONFIG_BT_HAS)
    target_sources(app PRIVATE btp_hap.c)
endif()

if(CONFIG_BT_CAP_INITIATOR)
    target_sources(app PRIVATE btp_cap.c)
endif()

if(CONFIG_BT_TBS)
    target_sources(app PRIVATE btp_ccp.c)
endif()

if(CONFIG_BT_TMAP)
    target_sources(app PRIVATE btp_tmap.c)
endif()

if(CONFIG_BT_PBP)
    target_sources(app PRIVATE btp_pbp.c)
endif()
