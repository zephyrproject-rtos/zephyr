if(CONFIG_WIFI_SIMPLELINK)
  zephyr_include_directories(
    .
    $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/kernel/zephyr/dpl
    $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source
    $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source/ti/drivers/net/wifi/porting
    $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source/ti/drivers/net/wifi/bsd
    )
  zephyr_sources(
    simplelink_support.c
    simplelink.c
    )
endif()
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD simplelink_sockets.c)
