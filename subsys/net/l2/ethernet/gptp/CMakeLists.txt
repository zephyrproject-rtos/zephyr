zephyr_list(SOURCES
  OUTPUT PRIVATE_SOURCES
  gptp.c
  gptp_user_api.c
  gptp_md.c
  gptp_messages.c
  gptp_mi.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
target_include_directories(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR})
