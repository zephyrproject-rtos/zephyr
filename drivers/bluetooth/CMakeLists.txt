zephyr_include_directories(${ZEPHYR_BASE}/subsys/bluetooth)
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BT_NRF51_PM} nrf51_pm.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

include_relative(hci/CMakeLists.txt)
