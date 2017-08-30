zephyr_sources(libc-hooks.c)

zephyr_include_directories(${LIBC_INCLUDE_DIR})
zephyr_link_libraries(
  m
  c
  -L${LIBC_LIBRARY_DIR}
  $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_PRINTF}>:-u_printf_float>
  $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_SCANF}>:-u_scanf_float>
  )
