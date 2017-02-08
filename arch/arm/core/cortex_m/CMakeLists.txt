target_sources(zephyr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/vector_table.S
  ${CMAKE_CURRENT_SOURCE_DIR}/reset.S
  ${CMAKE_CURRENT_SOURCE_DIR}/nmi_on_reset.S
  ${CMAKE_CURRENT_SOURCE_DIR}/prep_c.c
  ${CMAKE_CURRENT_SOURCE_DIR}/scb.c
  ${CMAKE_CURRENT_SOURCE_DIR}/nmi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/exc_manage.c
  )
