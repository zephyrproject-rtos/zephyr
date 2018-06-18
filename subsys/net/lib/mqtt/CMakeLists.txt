target_sources(subsys_net PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/mqtt.c
    ${CMAKE_CURRENT_LIST_DIR}/mqtt_pkt.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
