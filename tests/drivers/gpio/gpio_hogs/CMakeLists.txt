# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpio_hogs)

target_sources(app PRIVATE src/common.c)

if(CONFIG_GPIO_HOGS_INITIALIZE_BY_APPLICATION)
  target_sources(app PRIVATE src/init_by_application.c)
else()
  target_sources(app PRIVATE src/main.c)
endif()
