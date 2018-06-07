zephyr_sources(line_fifo.c)
zephyr_sources_ifdef(CONFIG_CONSOLE_GETCHAR getchar.c)
zephyr_sources_ifdef(CONFIG_CONSOLE_GETLINE getline.c)
