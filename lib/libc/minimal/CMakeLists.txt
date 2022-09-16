# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

zephyr_library()
zephyr_library_sources(
  source/stdlib/abort.c
  source/stdlib/atoi.c
  source/stdlib/strtol.c
  source/stdlib/strtoul.c
  source/stdlib/strtoll.c
  source/stdlib/strtoull.c
  source/stdlib/malloc.c
  source/stdlib/bsearch.c
  source/stdlib/exit.c
  source/stdlib/qsort.c
  source/string/strncasecmp.c
  source/string/strstr.c
  source/string/string.c
  source/string/strspn.c
  source/stdout/stdout_console.c
  source/stdout/sprintf.c
  source/stdout/fprintf.c
  source/math/sqrtf.c
  source/math/sqrt.c
)

if(CONFIG_MINIMAL_LIBC_TIME)
  zephyr_library_sources(source/time/gmtime.c)
  zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK source/time/time.c)
endif()

zephyr_library_sources_ifdef(CONFIG_MINIMAL_LIBC_RAND source/stdlib/rand.c)
