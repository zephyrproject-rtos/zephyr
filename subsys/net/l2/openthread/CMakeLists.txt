zephyr_list(SOURCES
    OUTPUT PRIVATE_SOURCES
    openthread.c
    openthread_utils.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
target_include_directories(subsys_net PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/openthread/platform
)
add_dependencies(subsys_net ot)
