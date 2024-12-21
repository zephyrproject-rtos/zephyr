#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of uut library.
#

add_library(uut STATIC
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/util/mem.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/util/memq.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_tx_queue.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_conn.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ll_feat.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_conn_upd.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_cc.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_pdu.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_remote.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_local.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_common.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_chmu.c
)

if (CONFIG_BT_CTLR_PHY)
  target_sources(uut PRIVATE
	  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_phy.c
	  )
endif()

if (CONFIG_BT_CTLR_LE_ENC)
  target_sources(uut PRIVATE
	  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_enc.c
	  )
endif()

if (CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER OR CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
  target_sources(uut PRIVATE
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_past.c
	  )
endif()


add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl mocks)

target_link_libraries(uut PUBLIC test_interface mocks)

add_definitions(-include ztest.h)
