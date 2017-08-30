zephyr_sources(libc-hooks.c)

zephyr_library_get_current_dir_lib_name(lib_name)
add_library(${lib_name} INTERFACE)
zephyr_link_libraries(${lib_name})

target_include_directories(${lib_name} INTERFACE ${LIBC_INCLUDE_DIR})
target_link_libraries(
  ${lib_name}
  INTERFACE
  m
  c
  -L${LIBC_LIBRARY_DIR}
  $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_PRINTF}>:-u_printf_float>
  $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_SCANF}>:-u_scanf_float>
  )
