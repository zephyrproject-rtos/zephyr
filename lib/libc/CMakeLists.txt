if(NOT CONFIG_ARCH_POSIX)

if(CONFIG_NEWLIB_LIBC)
  add_subdirectory(newlib)
else()
  add_subdirectory(minimal)
endif()

endif()
