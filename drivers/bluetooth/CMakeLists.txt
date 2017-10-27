zephyr_include_directories($ENV{ZEPHYR_BASE}/subsys/bluetooth)
add_subdirectory(hci)
zephyr_sources_ifdef(CONFIG_BT_NRF51_PM nrf51_pm.c)
