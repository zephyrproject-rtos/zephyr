target_sources(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tls_credentials.c)
target_include_directories(subsys_net PUBLIC ${CMAKE_CURRENT_LIST_DIR})
