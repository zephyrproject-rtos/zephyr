cmake_minimum_required(VERSION 3.13.1)

# Some boards could run this sample using frdm_kw41z shield, enforce
# -DSHIELD option in case they are used
if(BOARD STREQUAL mimxrt1020_evk OR
   BOARD STREQUAL mimxrt1050_evk OR
   BOARD STREQUAL mimxrt1060_evk OR
   BOARD STREQUAL frdm_k64f)
set(SHIELD frdm_kw41z)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(peripheral_hr)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
  ${app_sources}
  ../gatt/hrs.c
  ../gatt/bas.c
  )

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
