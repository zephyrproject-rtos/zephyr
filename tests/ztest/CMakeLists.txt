zephyr_include_directories(
  $ENV{ZEPHYR_BASE}/tests/include
  $ENV{ZEPHYR_BASE}/tests/ztest/include
  )

zephyr_sources(                           src/ztest.c)
zephyr_sources_ifdef(CONFIG_ZTEST_MOCKING src/ztest_mock.c)
