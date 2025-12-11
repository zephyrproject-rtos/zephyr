#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            id.c
            id_expects.c
            rpa.c
            conn.c
            hci_core.c
            hci_core_expects.c
            keys_help_utils.c
            util.c
            util_expects.c
            settings.c
            settings_store.c
            settings_expects.c
            settings_store_expects.c
)

target_include_directories(mocks PUBLIC
  ..
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
)

target_link_libraries(mocks PRIVATE test_interface)
