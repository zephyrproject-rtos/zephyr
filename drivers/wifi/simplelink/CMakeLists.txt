zephyr_include_directories(
  .
  $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/kernel/zephyr/dpl
  $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source
  $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source/ti/drivers/net/wifi/porting
  $ENV{ZEPHYR_BASE}/subsys/net/lib/tls_credentials
)
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            simplelink_support.c
            simplelink.c
            IFDEF:${CONFIG_NET_SOCKETS_OFFLOAD} simplelink_sockets.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
