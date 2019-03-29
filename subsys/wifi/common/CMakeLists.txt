# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(api.c)
zephyr_library_sources(cmd_prcs.c)
zephyr_library_sources(ctrl_iface.c)
zephyr_library_sources(drv_iface.c)
zephyr_library_sources(evt_lsnr.c)
zephyr_library_sources(notifier.c)
zephyr_library_sources(sm.c)
zephyr_library_sources(timer.c)

zephyr_library_sources_ifdef(CONFIG_WIFIMGR_CONFIG_SAVING config.c)
zephyr_library_sources_ifdef(CONFIG_WIFIMGR_LED led.c)
zephyr_library_sources_ifdef(CONFIG_WIFIMGR_PSK psk.c)

zephyr_library_link_libraries(subsys__wifi)

zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
