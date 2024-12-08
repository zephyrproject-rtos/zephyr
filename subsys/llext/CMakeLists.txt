if(CONFIG_LLEXT)
  zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/llext/llext.h)

  zephyr_library()

  # For strnlen()
  zephyr_library_compile_definitions(-D_POSIX_C_SOURCE=200809L)

  zephyr_library_sources(
		llext.c
		llext_mem.c
		llext_load.c
		llext_link.c
		llext_export.c
		llext_handlers.c
		buf_loader.c
    fs_loader.c
	)
  zephyr_library_sources_ifdef(CONFIG_LLEXT_SHELL shell.c)

  if(CONFIG_RISCV AND CONFIG_USERSPACE)
	message(WARNING "Running LLEXT extensions from user-space threads on RISC-V is not supported!")
  endif()
endif()
