zephyr_library()
zephyr_library_compile_definitions(${COMPILER})
zephyr_library_sources(altera_avalon_i2c.c)
set_source_files_properties(altera_avalon_i2c.c
  PROPERTIES COMPILE_DEFINITIONS "${COMPILER}")
set_source_files_properties(altera_avalon_i2c.c
  PROPERTIES COMPILE_FLAGS -Wno-maybe-uninitialized) # altera_avalon_i2c.c warns on uninitialized
