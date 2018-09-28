cmake_minimum_required(VERSION 3.8.2)
set(NO_QEMU_SERIAL_BT_SERVER 1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/bluetooth.c)
