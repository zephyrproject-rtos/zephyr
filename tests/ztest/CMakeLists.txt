zephyr_include_directories(
  $ENV{ZEPHYR_BASE}/tests/include
  $ENV{ZEPHYR_BASE}/tests/ztest/include
  )

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            src/ztest.c
            IFDEF:${CONFIG_ZTEST_MOCKING} src/ztest_mock.c
)

add_library(ztest STATIC ${PRIVATE_SOURCES})
# ztest provides the test_main symbol which is not seen as used by linker,
# thus -utest_main should be used during linking ztest.
target_link_libraries(ztest PRIVATE zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE ztest)
