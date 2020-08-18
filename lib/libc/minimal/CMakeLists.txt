# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

zephyr_library()
zephyr_library_sources(
  source/stdlib/atoi.c
  source/stdlib/strtol.c
  source/stdlib/strtoul.c
  source/stdlib/malloc.c
  source/stdlib/bsearch.c
  source/stdlib/exit.c
  source/string/strncasecmp.c
  source/string/strstr.c
  source/string/string.c
  source/string/strspn.c
  source/stdout/stdout_console.c
  source/stdout/sprintf.c
  source/stdout/fprintf.c
  source/time/gmtime.c
)
