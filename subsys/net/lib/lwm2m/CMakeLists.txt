# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(.)

zephyr_library_sources(
    lwm2m_engine.c
    lwm2m_registry.c
    lwm2m_observation.c
    lwm2m_message_handling.c
    lwm2m_obj_security.c
    lwm2m_obj_server.c
    lwm2m_obj_device.c
    lwm2m_rw_link_format.c
    lwm2m_rw_plain_text.c
    lwm2m_util.c
    lwm2m_rd_client.c
    )

# LWM2M Object Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_CONN_MON_OBJ_SUPPORT
    lwm2m_obj_connmon.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT
    lwm2m_obj_firmware.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_SWMGMT_OBJ_SUPPORT
    lwm2m_obj_swmgmt.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
    lwm2m_obj_firmware_pull.c
    lwm2m_pull_context.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_LOCATION_OBJ_SUPPORT
    lwm2m_obj_location.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_GATEWAY_OBJ_SUPPORT
    lwm2m_obj_gateway.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_PORTFOLIO_OBJ_SUPPORT
    lwm2m_obj_portfolio.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_EVENT_LOG_OBJ_SUPPORT
    lwm2m_obj_event_log.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_BINARYAPPDATA_OBJ_SUPPORT
    lwm2m_obj_binaryappdata.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_ACCESS_CONTROL_ENABLE
    lwm2m_obj_access_control.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_UCIFI_LPWAN
    ucifi_lpwan.c
    )

# TLV Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RW_OMA_TLV_SUPPORT
    lwm2m_rw_oma_tlv.c
    )
# JSON Support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RW_JSON_SUPPORT
    lwm2m_rw_json.c
    )
# SENML JSON support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT
    lwm2m_rw_senml_json.c
    )
# CBOR support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RW_CBOR_SUPPORT
    lwm2m_rw_cbor.c
    )
# SENML CBOR support
zephyr_library_sources_ifdef(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT
    lwm2m_rw_senml_cbor.c
    lwm2m_senml_cbor_decode.c
    lwm2m_senml_cbor_encode.c
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
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_CURRENT_SENSOR
    ipso_current_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_VOLTAGE_SENSOR
    ipso_voltage_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_IPSO_FILLING_SENSOR
    ipso_filling_sensor.c
    )
zephyr_library_sources_ifdef(CONFIG_LWM2M_UCIFI_BATTERY
    ucifi_battery.c
    )

# Shell commands
zephyr_library_sources_ifdef(CONFIG_LWM2M_SHELL
    lwm2m_shell.c
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
