target_include_directories_ifdef(CONFIG_CONSOLE_SHELL zephyr PUBLIC
	$ENV{ZEPHYR_BASE}/include/drivers)

zephyr_sources_ifdef(
        CONFIG_CONSOLE_SHELL
        shell_service.c shell.c)

add_subdirectory_ifdef(CONFIG_CONSOLE_SHELL modules)
