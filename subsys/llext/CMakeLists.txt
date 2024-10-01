if(CONFIG_LLEXT)
  zephyr_library()

  # For strnlen()
  zephyr_library_compile_definitions(-D_POSIX_C_SOURCE=200809L)

  zephyr_library_sources(
		llext.c
		llext_mem.c
		llext_load.c
		llext_link.c
		llext_export.c
		buf_loader.c
	)
  zephyr_library_sources_ifdef(CONFIG_LLEXT_SHELL shell.c)
endif()
