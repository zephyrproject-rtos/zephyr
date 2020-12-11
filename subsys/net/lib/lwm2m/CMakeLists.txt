# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(.)

zephyr_library_sources(
    lwm2m_engine.c
    lwm2m_obj_security.c
    lwm2m_obj_server.c
    lwm2m_obj_device.c
    lwm2m_rw_plain_text.c
    lwm2m_rw_oma_tlv.c
    lwm2m_util.c
    )

# LWM2M RD Client Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RD_CLIENT_SUPPORT
    lwm2m_rd_client.c
    )

# LWM2M Object Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_CONN_MON_OBJ_SUPPORT
    lwm2m_obj_connmon.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT
    lwm2m_obj_firmware.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
    lwm2m_obj_firmware_pull.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_LOCATION_OBJ_SUPPORT
    lwm2m_obj_location.c
    )

# JSON Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RW_JSON_SUPPORT
    lwm2m_rw_json.c
    )

# IPSO Objects
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_TEMP_SENSOR
    ipso_temp_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_GENERIC_SENSOR
    ipso_generic_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_HUMIDITY_SENSOR
    ipso_humidity_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_PRESSURE_SENSOR
    ipso_pressure_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_LIGHT_CONTROL
    ipso_light_control.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_ACCELEROMETER
    ipso_accelerometer.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_BUZZER
    ipso_buzzer.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_TIMER
    ipso_timer.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_ONOFF_SWITCH
    ipso_onoff_switch.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_PUSH_BUTTON
    ipso_push_button.c
    )

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
