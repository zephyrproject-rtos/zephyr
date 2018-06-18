zephyr_list(SOURCES
  OUTPUT PRIVATE_SOURCES
  lwm2m_engine.c
  lwm2m_obj_security.c
  lwm2m_obj_server.c
  lwm2m_obj_device.c
  lwm2m_rw_plain_text.c
  lwm2m_rw_oma_tlv.c
  IFDEF:${CONFIG_LWM2M_RD_CLIENT_SUPPORT}            lwm2m_rd_client.c
  IFDEF:${CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT}  lwm2m_obj_firmware.c
  IFDEF:${CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT} lwm2m_obj_firmware_pull.c
  IFDEF:${CONFIG_LWM2M_RW_JSON_SUPPORT}              lwm2m_rw_json.c
  IFDEF:${CONFIG_LWM2M_IPSO_TEMP_SENSOR}             ipso_temp_sensor.c
  IFDEF:${CONFIG_LWM2M_IPSO_LIGHT_CONTROL}           ipso_light_control.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
target_include_directories(subsys_net PUBLIC ${CMAKE_CURRENT_LIST_DIR})
