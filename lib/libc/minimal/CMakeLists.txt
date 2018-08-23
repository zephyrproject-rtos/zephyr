zephyr_include_directories(include)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            source/stdlib/atoi.c
            source/stdlib/strtol.c
            source/stdlib/strtoul.c
            source/stdlib/malloc.c
            source/string/strncasecmp.c
            source/string/strstr.c
            source/string/string.c
            source/stdout/prf.c
            source/stdout/stdout_console.c
            source/stdout/sprintf.c
            source/stdout/fprintf.c
)
target_sources(libc PRIVATE ${PRIVATE_SOURCES})
