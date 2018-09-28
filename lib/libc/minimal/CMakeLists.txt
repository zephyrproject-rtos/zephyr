zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(
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
