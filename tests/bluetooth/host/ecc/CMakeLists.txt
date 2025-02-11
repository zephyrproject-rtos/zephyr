#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            mocks/net_buf.c
            mocks/net_buf_expects.c
            mocks/hci_core.c
            mocks/hci_core_expects.c
            mocks/ecc_help_utils.c

            ${ZEPHYR_BASE}/subsys/bluetooth/host/ecc.c
            ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
)

target_include_directories(mocks PUBLIC
  .
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host/ecc/mocks
)

target_link_libraries(mocks PRIVATE test_interface)
