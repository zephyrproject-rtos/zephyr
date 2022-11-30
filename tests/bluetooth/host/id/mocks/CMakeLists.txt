#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            addr.c
            addr_expects.c
            adv.c
            conn.c
            crypto.c
            crypto_expects.c
            hci_core.c
            hci_core_expects.c
            kernel.c
            kernel_expects.c
            keys.c
            net_buf.c
            net_buf_expects.c
            rpa.c
            scan.c
            settings.c
            settings_expects.c
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
