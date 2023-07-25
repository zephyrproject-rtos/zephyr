# SPDX-License-Identifier: Apache-2.0

add_library(net_mocks STATIC
            assert.c
)

target_include_directories(net_mocks PUBLIC
  ${ZEPHYR_BASE}/subsys/net
  ${ZEPHYR_BASE}/tests/net
)

target_link_libraries(net_mocks PRIVATE test_interface)
target_compile_options(test_interface INTERFACE -include ztest.h)
