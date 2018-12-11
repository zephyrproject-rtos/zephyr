cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(watchdog)

if(CONFIG_IWDG_STM32)
target_sources(app PRIVATE src/iwdg_main.c)
else()
target_sources(app PRIVATE src/main.c)
endif()

