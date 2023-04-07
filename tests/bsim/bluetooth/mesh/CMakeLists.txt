# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (NOT DEFINED ENV{BSIM_COMPONENTS_PATH})
	message(FATAL_ERROR "This test requires the BabbleSim simulator. Please set\
 the  environment variable BSIM_COMPONENTS_PATH to point to its components \
 folder. More information can be found in\
 https://babblesim.github.io/folder_structure_and_env.html")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_mesh)

target_sources(app PRIVATE
 src/main.c
 src/mesh_test.c
)

if(CONFIG_BT_MESH_V1d1)
 target_sources(app PRIVATE
  src/dfu_blob_common.c
 )
endif()

if(CONFIG_SETTINGS)

 target_sources(app PRIVATE
  src/settings_test_backend.c
  src/test_persistence.c
  src/test_replay_cache.c
  src/test_provision.c
 )

 if(CONFIG_BT_MESH_V1d1)
  target_sources(app PRIVATE
   src/test_dfu.c
   src/test_blob.c
 )
 endif()

elseif(CONFIG_BT_MESH_GATT_PROXY)

 target_sources(app PRIVATE
  src/test_advertiser.c
 )

elseif(CONFIG_BT_CTLR_LOW_LAT)

 target_sources(app PRIVATE
  src/test_friendship.c
  src/test_transport.c
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
 )

 if(CONFIG_BT_MESH_V1d1)
  target_sources(app PRIVATE
   src/test_blob.c
   src/test_op_agg.c
   src/test_sar.c
   src/test_lcd.c
  )
 endif()

endif()

zephyr_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
