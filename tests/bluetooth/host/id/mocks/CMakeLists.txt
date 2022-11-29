#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            addr.c
            adv.c
            conn.c
            crypto.c
            hci_core.c
            kernel.c
            keys.c
            net_buf.c
            rpa.c
            scan.c
            settings.c
            smp.c
)

target_include_directories(mocks PUBLIC
  ..
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host/id/mocks
)

target_link_libraries(mocks PRIVATE test_interface)
