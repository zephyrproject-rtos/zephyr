zephyr_library()
target_link_libraries(subsys__net zephyr)

zephyr_library_sources(buf.c)

if(CONFIG_NETWORKING)
  if(CONFIG_NET_L2_RAW_CHANNEL)
    target_sources(subsys__net ip/nbuf.c)
  else()
    add_subdirectory(ip)
  endif()
endif()

add_subdirectory(lib)
