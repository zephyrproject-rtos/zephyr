zephyr_library()
zephyr_library_sources(buf.c)

if(CONFIG_NETWORKING)
  if(CONFIG_NET_L2_RAW_CHANNEL)
    zephyr_library_sources(ip/nbuf.c)
  else()
    add_subdirectory(ip)
  endif()
endif()

add_subdirectory(lib)
