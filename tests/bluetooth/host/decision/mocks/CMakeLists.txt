# SPDX-License-Identifier: Apache-2.0

add_library(mocks STATIC
            crypto.c
            hci_core.c
            kernel.c
            net_buf.c
            rpa.c
)

target_include_directories(mocks PUBLIC
  ..
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host/decision/mocks
)

target_link_libraries(mocks PRIVATE test_interface)
