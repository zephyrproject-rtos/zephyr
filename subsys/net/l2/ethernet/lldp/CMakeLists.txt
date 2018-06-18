target_include_directories(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR}
                                              ${ZEPHYR_BASE}/subsys/net/ip
)
target_sources(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lldp.c)
