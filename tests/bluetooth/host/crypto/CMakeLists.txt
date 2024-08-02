#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            mocks/kernel.c
            mocks/hci_core.c
            mocks/hci_core_expects.c
            mocks/aes.c
            mocks/aes_expects.c
            mocks/hmac_prng.c
            mocks/hmac_prng_expects.c
            mocks/crypto_help_utils.c

            ${ZEPHYR_BASE}/subsys/bluetooth/host/crypto_tc.c
            ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
            ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
            ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
)

target_include_directories(mocks PUBLIC
  .
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host/crypto/mocks
  ${ZEPHYR_BASE}/../modules/crypto/tinycrypt/lib/include
)

target_link_libraries(mocks PRIVATE test_interface)
