zephyr_sources_ifdef(CONFIG_FILE_SYSTEM_FAT   fat_fs.c)
zephyr_sources_ifdef(CONFIG_FILE_SYSTEM_NFFS  nffs_fs.c)
zephyr_sources_ifdef(CONFIG_FILE_SYSTEM_SHELL shell.c)
