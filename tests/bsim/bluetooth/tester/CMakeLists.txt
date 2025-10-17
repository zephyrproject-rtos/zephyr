# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tester_bsim)

# This contains babblesim-specific helpers, e.g. device synchronization.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )

zephyr_include_directories(
  src/
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/audio/btp
)

target_sources(app PRIVATE
  src/bsim_btp.c
  src/test_main.c
  src/audio/ccp_central.c
  src/audio/ccp_peripheral.c
  src/audio/csip_central.c
  src/audio/csip_peripheral.c
  src/audio/hap_central.c
  src/audio/hap_peripheral.c
  src/audio/mcp_central.c
  src/audio/mcp_peripheral.c
  src/audio/micp_central.c
  src/audio/micp_peripheral.c
  src/audio/tmap_central.c
  src/audio/tmap_peripheral.c
  src/audio/vcp_central.c
  src/audio/vcp_peripheral.c
  src/host/gap_central.c
  src/host/gap_peripheral.c
  src/host/iso_broadcaster.c
  src/host/iso_sync_receiver.c
)
