zephyr_library()
zephyr_library_sources(syscalls.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
