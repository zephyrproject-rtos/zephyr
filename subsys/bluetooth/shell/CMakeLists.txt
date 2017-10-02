zephyr_library()
zephyr_library_sources(
    bt.c
    gatt.c
    )
zephyr_library_sources_ifdef(CONFIG_BT_CTLR
    ll.c
    ticker.c
    )
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF5 flash.c)
