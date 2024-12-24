# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_mesh)

target_sources(app PRIVATE
 src/main.c
 src/mesh_test.c
 src/friendship_common.c
 src/gatt_common.c
 src/dfu_blob_common.c
)

if(CONFIG_SETTINGS)

 target_sources(app PRIVATE
  src/test_persistence.c
  src/test_replay_cache.c
  src/test_provision.c
  src/test_dfu.c
  src/test_blob.c
  src/test_sar.c
  src/test_lcd.c
  src/test_brg.c
)

if(CONFIG_BT_MESH_USES_MBEDTLS_PSA)
 target_sources(app PRIVATE
  src/distribute_keyid.c
  src/psa_its_emul.c
 )
endif()

if(CONFIG_BT_MESH_GATT_PROXY AND CONFIG_BT_MESH_PROXY_SOLICITATION)
 target_sources(app PRIVATE
  src/test_proxy_sol.c
 )
endif()

elseif(CONFIG_BT_MESH_GATT_PROXY)

 target_sources(app PRIVATE
  src/test_advertiser.c
  src/test_suspend.c
  src/test_beacon.c
  src/test_provision.c
 )

elseif(CONFIG_BT_CTLR_LOW_LAT)

 target_sources(app PRIVATE
  src/test_friendship.c
  src/test_transport.c
  src/test_suspend.c
  src/test_advertiser.c
 )

else()

 target_sources(app PRIVATE
  src/test_transport.c
  src/test_friendship.c
  src/test_provision.c
  src/test_beacon.c
  src/test_scanner.c
  src/test_heartbeat.c
  src/test_access.c
  src/test_iv_index.c
  src/test_advertiser.c
  src/test_suspend.c
  src/test_blob.c
  src/test_op_agg.c
  src/test_sar.c
  src/test_cdp1.c
  src/test_brg.c
 )

endif()

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)

# The mbedTLS PSA ITS is not thread safe.
# The issue: https://github.com/zephyrproject-rtos/zephyr/issues/59362
# Also, it isn't possible to use "native" ITS implementation since
# mbedTLS includes headers that do not exist.
# This linker option allows linking custom ITS implementation instead of
# precompiled objects from the mbedTLS library to run it in parallel.
if(CONFIG_BT_MESH_USES_MBEDTLS_PSA)
  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--allow-multiple-definition
    )
endif()
