target_include_directories_ifdef(CONFIG_ZTEST zephyr PUBLIC
	$ENV{ZEPHYR_BASE}/tests/include
	$ENV{ZEPHYR_BASE}/tests/ztest/include)

zephyr_sources_ifdef(CONFIG_ZTEST src/ztest.c)
zephyr_sources_ifdef(CONFIG_ZTEST_MOCKING src/ztest_mock.c)
