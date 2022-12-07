#
# CMakeLists.txt file for creating of host mocks library.
#

add_library(host_mocks STATIC
            host_mocks/assert.c
)

target_include_directories(host_mocks PUBLIC
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/host
  ${ZEPHYR_BASE}/tests/bluetooth/host
)

target_link_libraries(host_mocks PRIVATE test_interface)
target_compile_options(test_interface INTERFACE -include ztest.h)
