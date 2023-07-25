#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            mocks/net_buf.c
            mocks/iso.c
            mocks/hci_core.c
            mocks/net_buf_expects.c

            ${ZEPHYR_BASE}/subsys/bluetooth/host/buf.c
)

target_include_directories(mocks PUBLIC
  .
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/subsys/bluetooth
)

target_link_libraries(mocks PRIVATE test_interface)
