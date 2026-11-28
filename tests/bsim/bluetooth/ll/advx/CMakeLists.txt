# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_advx)

add_subdirectory("${ZEPHYR_BASE}/tests/bsim/babblekit" babblekit)
add_subdirectory("${ZEPHYR_BASE}/tests/bluetooth/common/testlib" testlib)
target_link_libraries(app PRIVATE
  babblekit
  testlib
)

target_sources(app PRIVATE
  "src/main.c"
  "src/aux_time.c"
  "src/scan_req_tgta.c"
  "src/anonymous.c"
  "src/connect.c"
)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/include
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic/lll
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/util
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/hal
  )

# src/anonymous.c checks the Direct_Address_Type of the HCI event in its own
# bt_hci_le_adv_ext_report(), which hands the event on to the one of the Host
set_source_files_properties(${ZEPHYR_BASE}/subsys/bluetooth/host/scan.c
  DIRECTORY ${ZEPHYR_BASE}/subsys/bluetooth/host
  PROPERTIES COMPILE_DEFINITIONS bt_hci_le_adv_ext_report=real_bt_hci_le_adv_ext_report)
