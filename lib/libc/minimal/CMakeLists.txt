target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdlib/atoi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdlib/strtol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdlib/strtoul.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/string/strncasecmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/string/strstr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/string/string.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdout/prf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdout/stdout_console.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdout/sprintf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdout/fprintf.c
)
zephyr_include_directories(include)
