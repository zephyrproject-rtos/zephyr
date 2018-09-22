cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(LoRaWAN)

target_sources(app PRIVATE src/main.c )
zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT src/power.c)
zephyr_sources_ifdef(CONFIG_DEVICE_POWER_MANAGEMENT src/device.c)
