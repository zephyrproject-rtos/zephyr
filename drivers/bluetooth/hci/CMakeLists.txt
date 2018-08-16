zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BT_H4}       h4.c
            IFDEF:${CONFIG_BT_H5}       h5.c
            IFDEF:${CONFIG_BT_SPI}      spi.c
            IFDEF:${CONFIG_BT_USERCHAN} userchan.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
