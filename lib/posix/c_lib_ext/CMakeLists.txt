# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(getopt)

if(CONFIG_TC_PROVIDES_POSIX_C_LIB_EXT)
  return()
endif()

zephyr_library()
zephyr_library_sources(
  fnmatch.c
  getentropy.c
  getopt/getopt.c
  getopt/getopt_common.c
)

zephyr_library_sources_ifdef(CONFIG_GETOPT_LONG getopt/getopt_long.c)
