target_include_directories(zephyr PUBLIC ${CMAKE_INCLUDE_CURRENT_DIR})

target_sources(zephyr PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/dns_pack.c
  ${CMAKE_CURRENT_LIST_DIR}/dns_client.c
)
