#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            mocks/id.c
            mocks/rpa.c
            mocks/conn.c
            mocks/hci_core.c
            mocks/hci_core_expects.c
            mocks/keys_help_utils.c

            ${ZEPHYR_BASE}/subsys/bluetooth/host/keys.c
            ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
)

target_include_directories(mocks PUBLIC
  .
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
  ${ZEPHYR_BASE}/include/zephyr/bluetooth
)

target_link_libraries(mocks PRIVATE test_interface)
