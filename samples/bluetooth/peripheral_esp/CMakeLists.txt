cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(peripheral_esp)

target_sources(app PRIVATE
  src/main.c
  ../gatt/bas.c
)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
