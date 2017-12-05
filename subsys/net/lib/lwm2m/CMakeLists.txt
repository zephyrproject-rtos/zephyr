zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_library()

zephyr_include_directories(.)

zephyr_library_sources(
    lwm2m_engine.c
    lwm2m_obj_security.c
    lwm2m_obj_server.c
    lwm2m_obj_device.c
    lwm2m_rw_plain_text.c
    lwm2m_rw_oma_tlv.c
    )

# LWM2M RD Client Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RD_CLIENT_SUPPORT
    lwm2m_rd_client.c
    )

# LWM2M Object Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT
    lwm2m_obj_firmware.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
    lwm2m_obj_firmware_pull.c
    )

# JSON Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RW_JSON_SUPPORT
    lwm2m_rw_json.c
    )

# IPSO Objects
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_TEMP_SENSOR
    ipso_temp_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_LIGHT_CONTROL
    ipso_light_control.c
    )

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
