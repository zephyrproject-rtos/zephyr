# SPDX-License-Identifier: Apache-2.0

# Only use if CONFIG_AUTH_LIB is defined as True
if(CONFIG_AUTH_LIB)
    zephyr_interface_library_named(authentication)
    zephyr_library()

    zephyr_library_sources_ifdef(CONFIG_AUTH_LIB auth_lib.c auth_xport_common.c)

    zephyr_library_sources_ifdef(CONFIG_BT_XPORT auth_xport_bt.c)

    zephyr_library_sources_ifdef(CONFIG_SERIAL_XPORT auth_xport_serial.c)

    zephyr_library_sources_ifdef(CONFIG_AUTH_CHALLENGE_RESPONSE auth_chalresp.c)

    zephyr_library_sources_ifdef(CONFIG_AUTH_DTLS auth_dtls.c)

    # Include Mbed if DTLS auth defined
    if(CONFIG_AUTH_DTLS)
        zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
    endif()

endif()
