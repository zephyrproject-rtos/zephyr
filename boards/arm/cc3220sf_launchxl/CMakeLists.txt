zephyr_library()
zephyr_library_sources(
    pinmux.c
    dbghdr.c
    )
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
