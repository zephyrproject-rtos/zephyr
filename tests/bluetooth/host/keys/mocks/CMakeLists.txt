#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            id.c
            rpa.c
            conn.c
            hci_core.c
            hci_core_expects.c
            keys_help_utils.c
)

target_include_directories(mocks PUBLIC
  ..
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
)

target_link_libraries(mocks PRIVATE test_interface)
