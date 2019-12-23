# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(  settings_func_test)

zephyr_include_directories(
	$ENV{ZEPHYR_BASE}/subsys/settings/include
	$ENV{ZEPHYR_BASE}/subsys/settings/src
	)

zephyr_library_sources(settings_basic_test.c)

# zephyr_library() is here in "app-mode", see
# https://github.com/zephyrproject-rtos/zephyr/issues/19582
# Random build failures without this, depends on the number of threads.
add_dependencies(      settings_func_test offsets_h)
