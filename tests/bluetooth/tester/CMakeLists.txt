# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

LIST(APPEND QEMU_EXTRA_FLAGS -serial unix:/tmp/bt-stack-tester)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tester)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/mesh)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/audio)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/host)
target_sources(app PRIVATE
    src/main.c
    src/btp.c
    src/btp_core.c
    src/btp_gap.c
    src/btp_gatt.c
    )
if(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
target_sources(app PRIVATE src/btp_l2cap.c)
endif()

if(CONFIG_BT_MESH)
target_sources(app PRIVATE src/btp_mesh.c)
endif()

target_sources_ifdef(CONFIG_BT_VCP_VOL_REND app PRIVATE src/btp_vcp.c)

if(CONFIG_BT_IAS OR CONFIG_BT_IAS_CLIENT)
target_sources(app PRIVATE src/btp_ias.c)
endif()

if(CONFIG_BT_BAP_UNICAST OR BT_BAP_BROADCAST_SOURCE OR BT_BAP_BROADCAST_SINK)
    target_sources(app PRIVATE
            src/btp_bap_audio_stream.c
            src/btp_bap.c
            )
endif()

if(CONFIG_BT_BAP_UNICAST)
    target_sources(app PRIVATE src/btp_bap_unicast.c)
endif()

if(CONFIG_BT_BAP_BROADCAST_SOURCE OR CONFIG_BT_BAP_BROADCAST_SOURCE)
    target_sources(app PRIVATE src/btp_bap_broadcast.c)
endif()

if(CONFIG_BT_HAS)
    target_sources(app PRIVATE src/btp_has.c)
endif()

if (CONFIG_BT_CSIP_SET_MEMBER)
    target_sources(app PRIVATE src/btp_csis.c)
endif()

if (CONFIG_BT_CSIP_SET_COORDINATOR)
    target_sources(app PRIVATE src/btp_csip.c)
endif()

if(CONFIG_BT_MICP_MIC_DEV)
    target_sources(app PRIVATE src/btp_micp.c)
endif()

if(CONFIG_BT_AICS)
    target_sources(app PRIVATE src/btp_aics.c)
endif()

if (CONFIG_BT_TBS_CLIENT)
    target_sources(app PRIVATE src/btp_ccp.c)
endif()

if (CONFIG_BT_CAP_ACCEPTOR)
    target_sources(app PRIVATE src/btp_cas.c)
endif()

if(CONFIG_BT_MCC OR CONFIG_BT_MCS)
    target_sources(app PRIVATE src/btp_mcp.c)
endif()

if(CONFIG_BT_HAS)
    target_sources(app PRIVATE src/btp_hap.c)
endif()

if(CONFIG_BT_CAP_INITIATOR)
    target_sources(app PRIVATE src/btp_cap.c)
endif()

if(CONFIG_BT_TBS OR CONFIG_BT_GTBS)
    target_sources(app PRIVATE src/btp_ccp.c)
endif()

if(CONFIG_BT_TMAP)
    target_sources(app PRIVATE src/btp_tmap.c)
endif()
