add_library(subsys__net STATIC buf.c)
zephyr_library(subsys__net)
target_link_libraries(subsys__net zephyr)

if(CONFIG_NETWORKING)
  if(CONFIG_NET_L2_RAW_CHANNEL)
    target_sources(subsys__net ip/nbuf.c)
  else()
    # TODO
    # add_subdirectory(ip)
  endif()
endif()

add_subdirectory(lib)
