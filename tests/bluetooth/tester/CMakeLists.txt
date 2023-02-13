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
